from __future__ import annotations

import copy
import json
from datetime import UTC, datetime, timedelta

import pytest
from sqlalchemy import func, select

from app.db.session import Database
from app.models import (
    AcousticMeasurement,
    Alert,
    Device,
    DeviceStatusHistory,
    IngestionFailure,
    Telemetry,
    Trip,
)
from app.services.ingestion import IngestionService
from app.websocket.manager import LiveUpdateManager
from tests.conftest import contract_fixture, settings_for


@pytest.fixture
async def ingestion(tmp_path):
    database = Database(f"sqlite+aiosqlite:///{tmp_path / 'ingestion.db'}")
    await database.create_schema()
    service = IngestionService(
        database.sessions,
        settings_for(
            f"sqlite+aiosqlite:///{tmp_path / 'ingestion.db'}",
            bootstrap_admin_email=None,
            bootstrap_admin_password=None,
        ),
        LiveUpdateManager(),
    )
    yield service, database
    await database.dispose()


def topic(branch: str) -> str:
    return f"vehiclesense/v1/vehicles/sim-vehicle-001/devices/sim-device-001/{branch}"


@pytest.mark.asyncio
async def test_persists_and_deduplicates_telemetry_and_quarantines_invalid(ingestion) -> None:
    service, database = ingestion
    payload = contract_fixture("telemetry_v3_full_simulated.json")
    first = await service.ingest(topic("telemetry"), json.dumps(payload))
    duplicate = await service.ingest(topic("telemetry"), json.dumps(payload))
    invalid = await service.ingest(topic("telemetry"), b"not-json")
    assert first.accepted and not first.duplicate
    assert duplicate.accepted and duplicate.duplicate
    assert not invalid.accepted

    async with database.sessions() as session:
        assert await session.scalar(select(func.count(Telemetry.id))) == 1
        assert await session.scalar(select(func.count(IngestionFailure.id))) == 1


@pytest.mark.asyncio
async def test_persists_acoustic_data_and_deduplicates_device_event(ingestion) -> None:
    service, database = ingestion
    acoustic = contract_fixture("acoustic_v1_simulated.json")
    event = contract_fixture("event_acoustic_simulated.json")
    assert (await service.ingest(topic("acoustic"), json.dumps(acoustic))).accepted
    assert (await service.ingest(topic("events"), json.dumps(event))).accepted
    duplicate = await service.ingest(topic("events"), json.dumps(event))
    assert duplicate.accepted and duplicate.duplicate

    async with database.sessions() as session:
        assert await session.scalar(select(func.count(AcousticMeasurement.id))) == 1
        assert await session.scalar(select(func.count(Alert.id))) == 1


@pytest.mark.asyncio
async def test_stale_offline_and_recovery_alert_lifecycle(ingestion) -> None:
    service, database = ingestion
    payload = contract_fixture("telemetry_v3_full_simulated.json")
    start = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)
    assert (await service.ingest(topic("telemetry"), json.dumps(payload), start)).accepted

    async with database.sessions() as session, session.begin():
        changed = await service.device_status.evaluate_stale_devices(
            session, start + timedelta(seconds=50)
        )
        assert [device.status for device in changed] == ["stale"]

    async with database.sessions() as session, session.begin():
        changed = await service.device_status.evaluate_stale_devices(
            session, start + timedelta(seconds=130)
        )
        assert [device.status for device in changed] == ["offline"]

    online = contract_fixture("status_online_simulated.json")
    recovered = await service.ingest(
        topic("status"), json.dumps(online), start + timedelta(seconds=140)
    )
    assert recovered.accepted

    async with database.sessions() as session:
        device = await session.scalar(select(Device))
        assert device is not None and device.status == "online"
        histories = list(
            (
                await session.scalars(
                    select(DeviceStatusHistory).order_by(DeviceStatusHistory.changed_at)
                )
            ).all()
        )
        assert [item.state for item in histories] == ["online", "stale", "offline", "online"]
        connectivity_alerts = list(
            (
                await session.scalars(
                    select(Alert).where(Alert.alert_type.in_(["telemetry_stale", "device_offline"]))
                )
            ).all()
        )
        assert {item.alert_type for item in connectivity_alerts} == {
            "telemetry_stale",
            "device_offline",
        }
        assert all(item.status == "resolved" for item in connectivity_alerts)


@pytest.mark.asyncio
async def test_temperature_alert_lifecycle_and_basic_trip_detection(ingestion) -> None:
    service, database = ingestion
    template = contract_fixture("telemetry_v3_full_simulated.json")
    start = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)

    async def sample(sequence: int, seconds: int, speed: float, temperature: float = 24.0):
        payload = copy.deepcopy(template)
        payload["sequence"] = sequence
        payload["sample_id"] = f"sim-device-001:7:{sequence}"
        payload["uptime_ms"] = sequence * 10_000
        payload["measured_at"] = (
            (start + timedelta(seconds=seconds)).isoformat().replace("+00:00", "Z")
        )
        payload["speed_kmh"] = speed
        payload["temperature_c"] = temperature
        payload["latitude"] += sequence * 0.0001
        return await service.ingest(
            topic("telemetry"),
            json.dumps(payload),
            received_at=start + timedelta(seconds=seconds),
        )

    await sample(1, 0, 30.0, 45.0)
    await sample(2, 10, 30.0, 45.0)
    await sample(3, 20, 30.0, 24.0)
    await sample(4, 30, 0.0, 24.0)
    await sample(5, 50, 0.0, 24.0)

    async with database.sessions() as session:
        trip = await session.scalar(select(Trip))
        assert trip is not None
        assert trip.status == "completed"
        assert trip.point_count >= 4
        alerts = list(
            (
                await session.scalars(
                    select(Alert).where(Alert.alert_type == "temperature_out_of_range")
                )
            ).all()
        )
        assert len(alerts) == 1
        assert alerts[0].status == "resolved"
