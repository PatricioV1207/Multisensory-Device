from __future__ import annotations

import asyncio
import contextlib
import logging
from contextlib import asynccontextmanager
from typing import Any
from uuid import UUID

import jwt
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from sqlalchemy import select

import app.models  # noqa: F401 - registers SQLAlchemy metadata
from app.api.v1.router import api_router
from app.core.config import Settings, get_settings
from app.core.logging import configure_logging, redact_connection_url
from app.core.security import decode_token, hash_password
from app.db.session import Database
from app.models import User, Vehicle
from app.mqtt.client import HiveMQService
from app.services.commands import dispatch_pending_commands
from app.services.ingestion import IngestionService
from app.services.time_utils import utc_now
from app.websocket.manager import LiveUpdateManager

logger = logging.getLogger(__name__)


async def bootstrap_admin(database: Database, settings: Settings) -> None:
    if not settings.bootstrap_admin_email or not settings.bootstrap_admin_password:
        return
    email = settings.bootstrap_admin_email.lower()
    password = settings.bootstrap_admin_password.get_secret_value()
    if not password:
        return
    if len(password) < 12:
        raise RuntimeError("bootstrap admin password must contain at least 12 characters")
    async with database.sessions() as session, session.begin():
        if await session.scalar(select(User.id).where(User.email == email)) is None:
            session.add(
                User(
                    email=email,
                    password_hash=hash_password(password),
                    role="admin",
                    active=True,
                )
            )
            logger.info("Bootstrap administrator created email=%s", email)


async def maintenance_loop(app: FastAPI) -> None:
    settings: Settings = app.state.settings
    interval = max(settings.status_scan_seconds, 1)
    while True:
        await asyncio.sleep(interval)
        try:
            status_updates: list[dict[str, Any]] = []
            async with app.state.database.sessions() as session, session.begin():
                changed = await app.state.ingestion.device_status.evaluate_stale_devices(
                    session, utc_now()
                )
                vehicles = {
                    vehicle.id: vehicle.vehicle_id
                    for vehicle in (await session.scalars(select(Vehicle))).all()
                }
                status_updates = [
                    {
                        "type": "device.status",
                        "vehicle_id": vehicles.get(device.vehicle_id),
                        "device_id": device.device_id,
                        "state": device.status,
                        "reason": device.status_reason,
                    }
                    for device in changed
                ]
                await dispatch_pending_commands(session, app.state.mqtt, settings, utc_now())
            for update in status_updates:
                await app.state.live_updates.broadcast(update, update["vehicle_id"])
        except asyncio.CancelledError:
            raise
        except Exception:
            logger.exception("Background status/command maintenance failed; retrying")


def create_app(settings: Settings | None = None) -> FastAPI:
    configured = settings or get_settings()
    configure_logging(configured.environment)

    @asynccontextmanager
    async def lifespan(app: FastAPI):
        database = Database(configured.database_url, echo=configured.sql_echo)
        app.state.settings = configured
        app.state.database = database
        if configured.auto_create_schema:
            await database.create_schema()
        await bootstrap_admin(database, configured)
        live_updates = LiveUpdateManager()
        ingestion = IngestionService(database.sessions, configured, live_updates)
        mqtt = HiveMQService(configured, ingestion)
        app.state.live_updates = live_updates
        app.state.ingestion = ingestion
        app.state.mqtt = mqtt
        mqtt.start(asyncio.get_running_loop())
        task = asyncio.create_task(maintenance_loop(app), name="vehiclesense-maintenance")
        app.state.maintenance_task = task
        logger.info(
            "VehicleSense backend started environment=%s database=%s",
            configured.environment,
            redact_connection_url(configured.database_url),
        )
        try:
            yield
        finally:
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task
            mqtt.stop()
            await database.dispose()

    app = FastAPI(
        title=configured.app_name,
        version="1.0.0",
        docs_url="/docs" if configured.environment != "production" else None,
        redoc_url=None,
        lifespan=lifespan,
    )
    app.add_middleware(
        CORSMiddleware,
        allow_origins=configured.cors_origins,
        allow_credentials=True,
        allow_methods=["GET", "POST", "PATCH", "DELETE", "OPTIONS"],
        allow_headers=["Authorization", "Content-Type"],
    )
    app.include_router(api_router, prefix=configured.api_prefix)

    @app.get("/health/live", tags=["health"])
    async def liveness() -> dict[str, str]:
        return {"status": "alive"}

    @app.get("/health/ready", tags=["health"])
    async def readiness():
        database_ready = await app.state.database.ping()
        mqtt_ready = not configured.mqtt_enabled or app.state.mqtt.connected
        ready = database_ready and mqtt_ready
        payload = {
            "status": "ready" if ready else "not_ready",
            "database": database_ready,
            "mqtt": {
                "enabled": configured.mqtt_enabled,
                "connected": app.state.mqtt.connected,
                "error": app.state.mqtt.last_error,
            },
        }
        return JSONResponse(payload, status_code=200 if ready else 503)

    @app.websocket("/ws/v1/live")
    async def live_socket(websocket: WebSocket):
        await websocket.accept()
        registered = False
        try:
            authentication = await asyncio.wait_for(websocket.receive_json(), timeout=5.0)
            if authentication.get("action") != "authenticate" or not isinstance(
                authentication.get("token"), str
            ):
                await websocket.close(code=4401, reason="authentication required")
                return
            token = authentication["token"]
            claims = decode_token(token, "access", configured)
            async with app.state.database.sessions() as session:
                user = await session.get(User, UUID(claims["sub"]))
            if user is None or not user.active:
                raise jwt.InvalidTokenError("inactive user")
        except TimeoutError:
            await websocket.close(code=4401, reason="authentication timeout")
            return
        except WebSocketDisconnect:
            return
        except (jwt.InvalidTokenError, ValueError):
            await websocket.close(code=4401, reason="invalid token")
            return
        await app.state.live_updates.register(websocket)
        registered = True
        await websocket.send_json({"type": "connection.ready", "role": user.role})
        try:
            while True:
                message = await websocket.receive_json()
                action = message.get("action")
                if action == "subscribe":
                    vehicle_ids = message.get("vehicle_ids", [])
                    if not isinstance(vehicle_ids, list) or not all(
                        isinstance(item, str) for item in vehicle_ids
                    ):
                        await websocket.send_json(
                            {"type": "error", "message": "vehicle_ids must be strings"}
                        )
                        continue
                    await app.state.live_updates.subscribe(websocket, vehicle_ids)
                    await websocket.send_json(
                        {"type": "subscription.updated", "vehicle_ids": vehicle_ids[:100]}
                    )
                elif action == "ping":
                    await websocket.send_json({"type": "pong"})
                else:
                    await websocket.send_json({"type": "error", "message": "unsupported action"})
        except WebSocketDisconnect:
            pass
        finally:
            if registered:
                await app.state.live_updates.disconnect(websocket)

    return app


app = create_app()
