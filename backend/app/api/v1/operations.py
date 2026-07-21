from __future__ import annotations

from datetime import timedelta
from uuid import uuid4

from fastapi import APIRouter, Depends, HTTPException, Request
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.api.dependencies import get_session, require_roles
from app.models import Command, Device, User, Vehicle
from app.schemas.fleet import CommandCreate
from app.services.commands import dispatch_pending_commands
from app.services.time_utils import utc_now

router = APIRouter(tags=["operations"])


@router.post("/commands", status_code=202)
async def create_command(
    payload: CommandCreate,
    request: Request,
    session: AsyncSession = Depends(get_session),
    user: User = Depends(require_roles("admin", "operator")),
) -> dict:
    device = await session.scalar(select(Device).where(Device.device_id == payload.device_id))
    if device is None:
        raise HTTPException(status_code=404, detail="device not found")
    vehicle = await session.get(Vehicle, device.vehicle_id)
    if vehicle is None:
        raise HTTPException(status_code=409, detail="device has no vehicle")
    now = utc_now()
    command = Command(
        command_id=f"cmd-{uuid4().hex}",
        device_id=device.id,
        command_type=payload.command_type,
        state="queued",
        requested_by=user.id,
        expires_at=now + timedelta(seconds=payload.expires_in_seconds),
        parameters=payload.parameters,
    )
    session.add(command)
    await session.flush()
    await dispatch_pending_commands(
        session, request.app.state.mqtt, request.app.state.settings, now
    )
    await session.commit()
    return {
        "command_id": command.command_id,
        "device_id": device.device_id,
        "vehicle_id": vehicle.vehicle_id,
        "state": command.state,
        "expires_at": command.expires_at,
    }


@router.get("/commands")
async def list_commands(
    session: AsyncSession = Depends(get_session),
    _: User = Depends(require_roles("admin", "operator", "viewer")),
) -> list[dict]:
    commands = list(
        (
            await session.scalars(select(Command).order_by(Command.created_at.desc()).limit(200))
        ).all()
    )
    return [
        {
            "command_id": item.command_id,
            "command": item.command_type,
            "state": item.state,
            "created_at": item.created_at,
            "expires_at": item.expires_at,
            "published_at": item.published_at,
            "acknowledged_at": item.acknowledged_at,
            "acknowledgement": item.acknowledgement,
        }
        for item in commands
    ]
