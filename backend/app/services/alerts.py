from __future__ import annotations

import math
from datetime import datetime
from typing import Any

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Alert, Device, Telemetry, Trip, Vehicle
from app.services.time_utils import ensure_utc

DEFAULT_THRESHOLDS = {
    "temperature_min_c": 5.0,
    "temperature_max_c": 40.0,
    "gps_invalid_seconds": 120,
    "gps_weak_hdop": 2.5,
    "microphone_invalid_seconds": 60,
    "acceleration_delta_mps2": 5.0,
    "prolonged_stop_seconds": 900,
}


class AlertService:
    async def evaluate_telemetry(
        self,
        session: AsyncSession,
        vehicle: Vehicle,
        device: Device,
        telemetry: Telemetry,
        observed_at: datetime,
    ) -> list[Alert]:
        observed_at = ensure_utc(observed_at)
        thresholds = {**DEFAULT_THRESHOLDS, **(vehicle.alert_thresholds or {})}
        changed: list[Alert] = []
        changed += await self._condition(
            session,
            vehicle,
            device,
            "temperature_out_of_range",
            telemetry.dht_valid
            and telemetry.temperature_c is not None
            and (
                telemetry.temperature_c < thresholds["temperature_min_c"]
                or telemetry.temperature_c > thresholds["temperature_max_c"]
            ),
            "high",
            "Temperatura fuera del rango configurado",
            observed_at,
            {"temperature_c": telemetry.temperature_c},
            telemetry.simulated,
        )

        if telemetry.gps_valid:
            device.gps_invalid_since = None
            gps_invalid_too_long = False
        else:
            device.gps_invalid_since = device.gps_invalid_since or observed_at
            gps_invalid_too_long = (
                observed_at - ensure_utc(device.gps_invalid_since)
            ).total_seconds() >= thresholds["gps_invalid_seconds"]
        changed += await self._condition(
            session,
            vehicle,
            device,
            "gps_invalid",
            gps_invalid_too_long,
            "medium",
            "GPS sin fix válido durante demasiado tiempo",
            observed_at,
            {
                "invalid_since": device.gps_invalid_since.isoformat()
                if device.gps_invalid_since
                else None
            },
            telemetry.simulated,
        )
        changed += await self._condition(
            session,
            vehicle,
            device,
            "gps_quality_weak",
            telemetry.gps_valid
            and telemetry.gps_hdop is not None
            and telemetry.gps_hdop > thresholds["gps_weak_hdop"],
            "low",
            "Calidad GPS débil",
            observed_at,
            {"hdop": telemetry.gps_hdop},
            telemetry.simulated,
        )

        invalid_sensors = [
            name
            for name, valid in {
                "dht11": telemetry.dht_valid,
                "adxl345": telemetry.accel_valid,
                "l3g4200d": telemetry.gyro_valid,
                "hmc5883l": telemetry.mag_valid,
                "bmp180": telemetry.baro_valid,
                "bh1750": telemetry.light_valid,
            }.items()
            if not valid
        ]
        changed += await self._condition(
            session,
            vehicle,
            device,
            "sensor_invalid",
            bool(invalid_sensors),
            "low",
            "Uno o más sensores no entregan datos válidos",
            observed_at,
            {"invalid_sensors": invalid_sensors},
            telemetry.simulated,
        )
        changed += await self._condition(
            session,
            vehicle,
            device,
            "storage_failure",
            not telemetry.storage_valid,
            "medium",
            "microSD no disponible para respaldo local",
            observed_at,
            {},
            telemetry.simulated,
        )

        if telemetry.microphone_valid:
            device.microphone_invalid_since = None
            microphone_invalid_too_long = False
        else:
            device.microphone_invalid_since = device.microphone_invalid_since or observed_at
            microphone_invalid_too_long = (
                observed_at - ensure_utc(device.microphone_invalid_since)
            ).total_seconds() >= thresholds["microphone_invalid_seconds"]
        changed += await self._condition(
            session,
            vehicle,
            device,
            "microphone_failure",
            microphone_invalid_too_long,
            "medium",
            "Micrófono INMP441 no válido",
            observed_at,
            {},
            telemetry.simulated,
        )
        clipping = bool(telemetry.raw_payload.get("acoustic_clipping", False))
        changed += await self._condition(
            session,
            vehicle,
            device,
            "microphone_clipping",
            telemetry.acoustic_valid and clipping,
            "medium",
            "Saturación acústica detectada",
            observed_at,
            {"relative_level_dbfs": telemetry.acoustic_level_dbfs},
            telemetry.simulated,
        )

        acceleration_candidate = False
        acceleration_magnitude = None
        if (
            telemetry.accel_valid
            and telemetry.accel_x is not None
            and telemetry.accel_y is not None
            and telemetry.accel_z is not None
        ):
            acceleration_magnitude = math.sqrt(
                telemetry.accel_x**2 + telemetry.accel_y**2 + telemetry.accel_z**2
            )
            acceleration_candidate = (
                abs(acceleration_magnitude - 9.80665) >= thresholds["acceleration_delta_mps2"]
            )
        changed += await self._condition(
            session,
            vehicle,
            device,
            "acceleration_candidate",
            acceleration_candidate,
            "medium",
            "Candidato de aceleración o frenado brusco",
            observed_at,
            {"magnitude_mps2": acceleration_magnitude},
            telemetry.simulated,
        )

        prolonged_stop = (
            device.stopped_candidate_since is not None
            and (observed_at - ensure_utc(device.stopped_candidate_since)).total_seconds()
            >= thresholds["prolonged_stop_seconds"]
        )
        changed += await self._condition(
            session,
            vehicle,
            device,
            "prolonged_stop",
            prolonged_stop,
            "low",
            "Vehículo detenido durante un periodo prolongado",
            observed_at,
            {},
            telemetry.simulated,
        )
        return changed

    async def create_external_event(
        self,
        session: AsyncSession,
        vehicle: Vehicle,
        device: Device,
        payload: dict[str, Any],
        observed_at: datetime,
    ) -> tuple[Alert, bool]:
        existing = await session.scalar(
            select(Alert).where(Alert.external_event_id == payload["event_id"])
        )
        if existing is not None:
            return existing, True
        trip = await session.scalar(
            select(Trip).where(Trip.vehicle_id == vehicle.id, Trip.status == "active").limit(1)
        )
        alert = Alert(
            event_key=f"device-event:{payload['event_id']}",
            external_event_id=payload["event_id"],
            vehicle_id=vehicle.id,
            device_id=device.id,
            trip_id=trip.id if trip else None,
            alert_type=payload["event_type"],
            severity=payload["severity"],
            status="active",
            source="device",
            triggered_at=observed_at,
            last_observed_at=observed_at,
            title=payload["event_type"].replace("_", " ").capitalize(),
            details=payload.get("details", {}),
            simulated=payload.get("simulated", False),
        )
        session.add(alert)
        return alert, False

    async def set_connectivity_alert(
        self,
        session: AsyncSession,
        vehicle: Vehicle,
        device: Device,
        state: str,
        observed_at: datetime,
        reason: str | None,
    ) -> list[Alert]:
        changed: list[Alert] = []
        changed += await self._condition(
            session,
            vehicle,
            device,
            "device_offline",
            state == "offline",
            "high",
            "Dispositivo desconectado",
            observed_at,
            {"reason": reason},
            device.simulated,
        )
        changed += await self._condition(
            session,
            vehicle,
            device,
            "telemetry_stale",
            state == "stale",
            "medium",
            "Telemetría desactualizada",
            observed_at,
            {"reason": reason},
            device.simulated,
        )
        if state == "online":
            changed += await self._condition(
                session,
                vehicle,
                device,
                "device_offline",
                False,
                "high",
                "Dispositivo desconectado",
                observed_at,
                {},
                device.simulated,
            )
            changed += await self._condition(
                session,
                vehicle,
                device,
                "telemetry_stale",
                False,
                "medium",
                "Telemetría desactualizada",
                observed_at,
                {},
                device.simulated,
            )
        return changed

    async def _condition(
        self,
        session: AsyncSession,
        vehicle: Vehicle,
        device: Device,
        alert_type: str,
        active: bool,
        severity: str,
        title: str,
        observed_at: datetime,
        details: dict[str, Any],
        simulated: bool,
    ) -> list[Alert]:
        current = await session.scalar(
            select(Alert)
            .where(
                Alert.device_id == device.id,
                Alert.alert_type == alert_type,
                Alert.status.in_(["active", "acknowledged"]),
            )
            .order_by(Alert.triggered_at.desc())
            .limit(1)
        )
        if not active:
            if current is None:
                return []
            current.status = "resolved"
            current.resolved_at = observed_at
            current.last_observed_at = observed_at
            return [current]
        if current is not None:
            current.last_observed_at = observed_at
            current.occurrence_count += 1
            current.details = details
            return [current]
        event_key = (
            f"condition:{device.device_id}:{alert_type}:"
            f"{observed_at.isoformat(timespec='microseconds')}"
        )
        trip = await session.scalar(
            select(Trip).where(Trip.vehicle_id == vehicle.id, Trip.status == "active").limit(1)
        )
        created = Alert(
            event_key=event_key,
            vehicle_id=vehicle.id,
            device_id=device.id,
            trip_id=trip.id if trip else None,
            alert_type=alert_type,
            severity=severity,
            status="active",
            source="backend",
            triggered_at=observed_at,
            last_observed_at=observed_at,
            title=title,
            details=details,
            simulated=simulated,
        )
        session.add(created)
        return [created]
