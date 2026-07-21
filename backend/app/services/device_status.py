from __future__ import annotations

from datetime import datetime

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.core.config import Settings
from app.models import Device, DeviceStatusHistory, Vehicle
from app.services.alerts import AlertService
from app.services.time_utils import ensure_utc


class DeviceStatusService:
    def __init__(self, settings: Settings, alerts: AlertService) -> None:
        self.settings = settings
        self.alerts = alerts

    async def mark(
        self,
        session: AsyncSession,
        device: Device,
        vehicle: Vehicle,
        state: str,
        observed_at: datetime,
        reason: str | None,
        payload: dict | None = None,
    ) -> None:
        observed_at = ensure_utc(observed_at)
        changed = device.status != state or device.status_reason != reason
        device.status = state
        device.status_reason = reason
        device.last_status_at = observed_at
        if state in {"online", "reconnecting"}:
            device.last_seen_at = observed_at
        if changed:
            session.add(
                DeviceStatusHistory(
                    device_id=device.id,
                    state=state,
                    reason=reason,
                    changed_at=observed_at,
                    payload=payload or {},
                )
            )
        await self.alerts.set_connectivity_alert(
            session, vehicle, device, state, observed_at, reason
        )

    async def evaluate_stale_devices(self, session: AsyncSession, now: datetime) -> list[Device]:
        changed: list[Device] = []
        devices = list((await session.scalars(select(Device))).all())
        for device in devices:
            if device.last_seen_at is None:
                continue
            age = (ensure_utc(now) - ensure_utc(device.last_seen_at)).total_seconds()
            target = (
                "offline"
                if age >= self.settings.offline_after_seconds
                else "stale"
                if age >= self.settings.stale_after_seconds
                else "online"
            )
            if device.status == target:
                continue
            vehicle = await session.get(Vehicle, device.vehicle_id)
            if vehicle is None:
                continue
            reason = (
                "telemetry_timeout"
                if target == "offline"
                else "telemetry_stale"
                if target == "stale"
                else "telemetry_resumed"
            )
            await self.mark(session, device, vehicle, target, now, reason)
            changed.append(device)
        return changed
