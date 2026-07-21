from __future__ import annotations

import hashlib
import json
import logging
from dataclasses import dataclass
from datetime import datetime
from typing import Any

from sqlalchemy import select
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.core.config import Settings
from app.models import (
    AcousticMeasurement,
    Command,
    Device,
    IngestionFailure,
    Telemetry,
    Vehicle,
)
from app.services.alerts import AlertService
from app.services.contract_validator import (
    ContractValidationError,
    ContractValidator,
    ValidatedMessage,
)
from app.services.device_status import DeviceStatusService
from app.services.time_utils import parse_contract_time, utc_now
from app.services.trips import TripService
from app.websocket.manager import LiveUpdateManager

logger = logging.getLogger(__name__)


@dataclass(frozen=True, slots=True)
class IngestionOutcome:
    accepted: bool
    duplicate: bool = False
    message_type: str | None = None
    entity_id: str | None = None
    error: str | None = None


class IngestionService:
    def __init__(
        self,
        sessions: async_sessionmaker,
        settings: Settings,
        live_updates: LiveUpdateManager,
    ) -> None:
        self.sessions = sessions
        self.settings = settings
        self.validator = ContractValidator(settings)
        self.live_updates = live_updates
        self.alerts = AlertService()
        self.trips = TripService(settings)
        self.device_status = DeviceStatusService(settings, self.alerts)

    async def ingest(
        self,
        topic: str,
        raw_payload: bytes | str,
        received_at: datetime | None = None,
    ) -> IngestionOutcome:
        received_at = received_at or utc_now()
        try:
            message = self.validator.validate(topic, raw_payload)
            async with self.sessions() as session, session.begin():
                vehicle, device = await self._resolve_identity(session, message)
                if message.message_type == "telemetry":
                    outcome, update = await self._telemetry(
                        session, vehicle, device, message, received_at
                    )
                elif message.message_type == "acoustic":
                    outcome, update = await self._acoustic(
                        session, vehicle, device, message.payload, received_at
                    )
                elif message.message_type == "event":
                    outcome, update = await self._event(
                        session, vehicle, device, message.payload, received_at
                    )
                elif message.message_type == "status":
                    outcome, update = await self._status(
                        session, vehicle, device, message.payload, received_at
                    )
                elif message.message_type == "command_ack":
                    outcome, update = await self._command_ack(
                        session, vehicle, device, message.payload, received_at
                    )
                else:
                    raise ContractValidationError("validated message type has no handler")
            if update is not None:
                await self.live_updates.broadcast(update, message.vehicle_id)
            return outcome
        except ContractValidationError as error:
            await self._quarantine(topic, raw_payload, str(error), received_at)
            logger.warning("MQTT message quarantined topic=%s error=%s", topic, error)
            return IngestionOutcome(accepted=False, error=str(error))
        except Exception:
            await self._quarantine(topic, raw_payload, "internal ingestion failure", received_at)
            logger.exception("MQTT ingestion failed topic=%s", topic)
            return IngestionOutcome(accepted=False, error="internal ingestion failure")

    async def _resolve_identity(self, session, message: ValidatedMessage):
        vehicle = await session.scalar(
            select(Vehicle).where(Vehicle.vehicle_id == message.vehicle_id)
        )
        device = await session.scalar(select(Device).where(Device.device_id == message.device_id))
        if vehicle is None or device is None:
            if not self.settings.auto_register_devices:
                raise ContractValidationError("vehicle or device is not registered")
            if vehicle is None:
                vehicle = Vehicle(
                    vehicle_id=message.vehicle_id,
                    display_name=message.vehicle_id,
                    metadata_json={"auto_registered": True},
                )
                session.add(vehicle)
                await session.flush()
            if device is None:
                device = Device(
                    device_id=message.device_id,
                    vehicle_id=vehicle.id,
                    display_name=message.device_id,
                    status="unknown",
                    simulated=bool(message.payload.get("simulated", False)),
                )
                session.add(device)
                await session.flush()
        if device.vehicle_id != vehicle.id:
            raise ContractValidationError("device is assigned to a different vehicle")
        return vehicle, device

    async def _telemetry(
        self,
        session,
        vehicle: Vehicle,
        device: Device,
        message: ValidatedMessage,
        received_at: datetime,
    ):
        payload = message.payload
        version = int(payload["schema_version"])
        sample_id = payload.get("sample_id")
        if version == 2:
            canonical = json.dumps(payload, sort_keys=True, separators=(",", ":"))
            digest = hashlib.sha256(canonical.encode()).hexdigest()
            dedupe_key = f"legacy:{device.device_id}:{digest}"
        else:
            dedupe_key = sample_id
        existing = await session.scalar(
            select(Telemetry.id).where(Telemetry.dedupe_key == dedupe_key)
        )
        if existing is not None:
            return (
                IngestionOutcome(True, True, "telemetry", str(existing)),
                None,
            )

        measured_at = (
            parse_contract_time(payload.get("measured_at")) if payload.get("time_valid") else None
        )
        telemetry = Telemetry(
            sample_id=sample_id,
            dedupe_key=dedupe_key,
            schema_version=version,
            legacy=version == 2,
            device_id=device.id,
            vehicle_id=vehicle.id,
            boot_id=payload.get("boot_id"),
            sequence=payload.get("sequence"),
            measured_at=measured_at,
            received_at=received_at,
            uptime_ms=payload["uptime_ms"],
            replayed=payload.get("replayed", False),
            simulated=payload.get("simulated", False),
            dht_valid=payload.get("dht_valid", False),
            gps_valid=payload.get("gps_valid", False),
            accel_valid=payload.get("accel_valid", False),
            gyro_valid=payload.get("gyro_valid", False),
            mag_valid=payload.get("mag_valid", False),
            baro_valid=payload.get("baro_valid", False),
            imu_valid=payload.get("imu_valid", False),
            light_valid=payload.get("bh1750_valid", False),
            storage_valid=payload.get("sd_valid", False),
            microphone_valid=payload.get("mic_valid", False),
            acoustic_valid=payload.get("acoustic_valid", False),
            temperature_c=payload.get("temperature_c"),
            humidity_percent=payload.get("humidity_percent"),
            latitude=payload.get("latitude"),
            longitude=payload.get("longitude"),
            gps_altitude_m=payload.get("altitude_m"),
            speed_kmh=payload.get("speed_kmh"),
            satellites=payload.get("satellites"),
            gps_hdop=payload.get("gps_hdop"),
            pressure_hpa=payload.get("pressure_hpa"),
            baro_altitude_m=payload.get("baro_altitude_m"),
            light_lux=payload.get("light_lux"),
            accel_x=payload.get("accel_x"),
            accel_y=payload.get("accel_y"),
            accel_z=payload.get("accel_z"),
            gyro_x=payload.get("gyro_x"),
            gyro_y=payload.get("gyro_y"),
            gyro_z=payload.get("gyro_z"),
            mag_x=payload.get("mag_x"),
            mag_y=payload.get("mag_y"),
            mag_z=payload.get("mag_z"),
            acoustic_level_dbfs=payload.get("acoustic_relative_level_dbfs"),
            acoustic_category=payload.get("acoustic_category"),
            acoustic_confidence=payload.get("acoustic_confidence"),
            raw_payload=payload,
        )
        session.add(telemetry)
        await session.flush()
        event_time = measured_at or received_at
        await self.device_status.mark(
            session,
            device,
            vehicle,
            "online",
            received_at,
            "telemetry_received",
        )
        device.last_boot_id = payload.get("boot_id", device.last_boot_id)
        device.simulated = telemetry.simulated
        trip = await self.trips.process(session, device, telemetry, event_time)
        alerts = await self.alerts.evaluate_telemetry(
            session, vehicle, device, telemetry, event_time
        )
        update = {
            "type": "telemetry.received",
            "vehicle_id": vehicle.vehicle_id,
            "device_id": device.device_id,
            "sample_id": sample_id,
            "received_at": received_at.isoformat(),
            "data": payload,
            "trip_id": str(trip.id) if trip else None,
            "alerts_changed": [str(alert.id) for alert in alerts],
        }
        return IngestionOutcome(True, False, "telemetry", str(telemetry.id)), update

    async def _acoustic(
        self,
        session,
        vehicle: Vehicle,
        device: Device,
        payload: dict[str, Any],
        received_at: datetime,
    ):
        existing = await session.scalar(
            select(AcousticMeasurement.id).where(
                AcousticMeasurement.sample_id == payload["sample_id"]
            )
        )
        if existing is not None:
            return IngestionOutcome(True, True, "acoustic", str(existing)), None
        measured_at = (
            parse_contract_time(payload.get("measured_at")) if payload.get("time_valid") else None
        )
        features = {
            key: payload[key]
            for key in (
                "window_duration_ms",
                "sample_rate_hz",
                "clipping_ratio",
                "crest_factor",
                "zero_crossing_rate",
                "spectral_centroid_hz",
                "spectral_flatness",
                "spectral_rolloff_hz",
                "band_energy_ratio",
            )
            if key in payload
        }
        record = AcousticMeasurement(
            sample_id=payload["sample_id"],
            device_id=device.id,
            vehicle_id=vehicle.id,
            measured_at=measured_at,
            received_at=received_at,
            microphone_valid=payload["mic_valid"],
            analysis_valid=payload["analysis_valid"],
            relative_level_dbfs=payload.get("relative_level_dbfs"),
            peak_dbfs=payload.get("peak_dbfs"),
            clipping=payload.get("clipping"),
            category=payload.get("category"),
            confidence=payload.get("confidence"),
            classifier_version=payload.get("classifier_version"),
            features=features,
            simulated=payload.get("simulated", False),
            raw_payload=payload,
        )
        session.add(record)
        await session.flush()
        await self.device_status.mark(
            session, device, vehicle, "online", received_at, "acoustic_received"
        )
        update = {
            "type": "acoustic.received",
            "vehicle_id": vehicle.vehicle_id,
            "device_id": device.device_id,
            "sample_id": record.sample_id,
            "received_at": received_at.isoformat(),
            "data": payload,
        }
        return IngestionOutcome(True, False, "acoustic", str(record.id)), update

    async def _event(
        self,
        session,
        vehicle: Vehicle,
        device: Device,
        payload: dict[str, Any],
        received_at: datetime,
    ):
        event_time = (
            parse_contract_time(payload.get("occurred_at"))
            if payload.get("time_valid")
            else received_at
        ) or received_at
        alert, duplicate = await self.alerts.create_external_event(
            session, vehicle, device, payload, event_time
        )
        await session.flush()
        if duplicate:
            return IngestionOutcome(True, True, "event", str(alert.id)), None
        update = {
            "type": "alert.changed",
            "vehicle_id": vehicle.vehicle_id,
            "device_id": device.device_id,
            "alert_id": str(alert.id),
            "data": payload,
        }
        return IngestionOutcome(True, False, "event", str(alert.id)), update

    async def _status(
        self,
        session,
        vehicle: Vehicle,
        device: Device,
        payload: dict[str, Any],
        received_at: datetime,
    ):
        state = payload["state"]
        device.firmware_version = payload.get("firmware_version", device.firmware_version)
        device.last_boot_id = payload.get("boot_id", device.last_boot_id)
        device.simulated = payload.get("simulated", device.simulated)
        await self.device_status.mark(
            session,
            device,
            vehicle,
            state,
            received_at,
            payload.get("reason"),
            payload,
        )
        update = {
            "type": "device.status",
            "vehicle_id": vehicle.vehicle_id,
            "device_id": device.device_id,
            "state": state,
            "reason": payload.get("reason"),
            "received_at": received_at.isoformat(),
        }
        return IngestionOutcome(True, False, "status", str(device.id)), update

    async def _command_ack(
        self,
        session,
        vehicle: Vehicle,
        device: Device,
        payload: dict[str, Any],
        received_at: datetime,
    ):
        command = await session.scalar(
            select(Command).where(Command.command_id == payload["command_id"])
        )
        if command is None or command.device_id != device.id:
            raise ContractValidationError("command acknowledgement has no matching command")
        duplicate = command.acknowledged_at is not None
        command.state = payload["state"]
        command.acknowledged_at = received_at
        command.acknowledgement = payload
        update = {
            "type": "command.acknowledged",
            "vehicle_id": vehicle.vehicle_id,
            "device_id": device.device_id,
            "command_id": command.command_id,
            "state": command.state,
        }
        return IngestionOutcome(True, duplicate, "command_ack", str(command.id)), update

    async def _quarantine(
        self,
        topic: str,
        raw_payload: bytes | str,
        error: str,
        received_at: datetime,
    ) -> None:
        preview = (
            raw_payload.decode("utf-8", errors="replace")
            if isinstance(raw_payload, bytes)
            else raw_payload
        )[:4096]
        try:
            async with self.sessions() as session, session.begin():
                session.add(
                    IngestionFailure(
                        topic=topic[:512],
                        payload_preview=preview,
                        error=error[:2000],
                        received_at=received_at,
                    )
                )
        except Exception:
            logger.exception("Could not persist ingestion quarantine record")
