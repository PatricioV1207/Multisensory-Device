from __future__ import annotations

from datetime import datetime
from typing import Any

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.core.config import Settings
from app.models import Command, Device, User, Vehicle
from app.mqtt.client import HiveMQService
from app.services.time_utils import ensure_utc, utc_now


def command_topic(settings: Settings, vehicle_id: str, device_id: str) -> str:
    prefix = settings.mqtt_topic_prefix.rstrip("/")
    return f"{prefix}/vehicles/{vehicle_id}/devices/{device_id}/commands"


def command_payload(
    command: Command,
    device: Device,
    vehicle: Vehicle,
    requested_by: User | None,
    issued_at: datetime,
) -> dict[str, Any]:
    return {
        "schema_version": 1,
        "message_type": "command",
        "command_id": command.command_id,
        "device_id": device.device_id,
        "vehicle_id": vehicle.vehicle_id,
        "command": command.command_type,
        "issued_at": ensure_utc(issued_at).isoformat().replace("+00:00", "Z"),
        "expires_at": ensure_utc(command.expires_at).isoformat().replace("+00:00", "Z"),
        "parameters": command.parameters,
        "requested_by": str(requested_by.id if requested_by else command.requested_by),
    }


async def dispatch_pending_commands(
    session: AsyncSession,
    mqtt: HiveMQService,
    settings: Settings,
    now: datetime | None = None,
) -> int:
    now = now or utc_now()
    commands = list(
        (
            await session.scalars(
                select(Command)
                .where(Command.state == "queued")
                .order_by(Command.created_at)
                .limit(50)
            )
        ).all()
    )
    sent = 0
    for command in commands:
        if ensure_utc(command.expires_at) <= ensure_utc(now):
            command.state = "expired"
            continue
        device = await session.get(Device, command.device_id)
        vehicle = await session.get(Vehicle, device.vehicle_id) if device else None
        requested_by = await session.get(User, command.requested_by)
        if device is None or vehicle is None:
            command.state = "failed"
            continue
        payload = command_payload(
            command,
            device,
            vehicle,
            requested_by,
            command.created_at or now,
        )
        if mqtt.publish_command(
            command_topic(settings, vehicle.vehicle_id, device.device_id), payload
        ):
            command.state = "published"
            command.published_at = now
            sent += 1
    return sent
