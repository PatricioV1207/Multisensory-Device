from __future__ import annotations

import asyncio
from unittest.mock import AsyncMock

import paho.mqtt.client as mqtt
import pytest

from app.mqtt.client import HiveMQService
from app.services.ingestion import IngestionOutcome
from tests.conftest import settings_for


class FakeClient:
    def __init__(self) -> None:
        self.acknowledged: list[tuple[int, int]] = []

    def ack(self, message_id: int, qos: int):
        self.acknowledged.append((message_id, qos))
        return mqtt.MQTT_ERR_SUCCESS


@pytest.mark.asyncio
async def test_acknowledges_persisted_or_quarantined_messages() -> None:
    service = HiveMQService(settings_for("sqlite+aiosqlite:///:memory:"), AsyncMock())
    client = FakeClient()
    service._client = client
    persisted = asyncio.get_running_loop().create_future()
    persisted.set_result(IngestionOutcome(accepted=True))
    service._finish_ingestion(persisted, 41, 1)
    quarantined = asyncio.get_running_loop().create_future()
    quarantined.set_result(IngestionOutcome(accepted=False, error="schema validation failed"))
    service._finish_ingestion(quarantined, 42, 1)
    assert client.acknowledged == [(41, 1), (42, 1)]


@pytest.mark.asyncio
async def test_database_failure_withholds_ack_and_requests_redelivery() -> None:
    service = HiveMQService(settings_for("sqlite+aiosqlite:///:memory:"), AsyncMock())
    client = FakeClient()
    service._client = client
    service._loop = asyncio.get_running_loop()
    service._reconnect_after_ingestion_failure = AsyncMock()
    failed = asyncio.get_running_loop().create_future()
    failed.set_result(IngestionOutcome(accepted=False, error="internal ingestion failure"))
    service._finish_ingestion(failed, 43, 1)
    await asyncio.sleep(0)
    assert client.acknowledged == []
    service._reconnect_after_ingestion_failure.assert_awaited_once()
