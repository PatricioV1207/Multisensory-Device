from __future__ import annotations

from datetime import UTC, datetime, timedelta

import pytest

from vehiclesense_simulator.config import SimulatorSettings
from vehiclesense_simulator.contracts import ContractRegistry
from vehiclesense_simulator.generator import Envelope, VehicleGenerator
from vehiclesense_simulator.main import run
from vehiclesense_simulator.publisher import DevicePublisher


@pytest.mark.parametrize("scenario", ["normal", "mixed", "stress"])
def test_every_generated_message_matches_its_contract(scenario: str) -> None:
    registry = ContractRegistry()
    generator = VehicleGenerator(1, seed=73, registry=registry)
    start = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)
    for cycle in range(1, 81):
        for envelope in generator.next_cycle(start + timedelta(seconds=cycle * 2), scenario):
            assert envelope.validate
            registry.validate(envelope.payload)
            assert envelope.payload["simulated"] is True
            assert generator.vehicle_id in envelope.topic
            assert generator.device_id in envelope.topic


def test_gps_and_microphone_invalidity_omit_dependent_values() -> None:
    generator = VehicleGenerator(2, seed=42)
    start = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)
    gps_invalid = None
    for cycle in range(1, 50):
        envelopes = generator.next_cycle(start + timedelta(seconds=cycle), "mixed")
        telemetry = next(
            (item.payload for item in envelopes if item.payload.get("message_type") == "telemetry"),
            None,
        )
        if telemetry and not telemetry["gps_valid"]:
            gps_invalid = telemetry
            break
    assert gps_invalid is not None
    assert "latitude" not in gps_invalid
    assert "speed_kmh" not in gps_invalid

    microphone = VehicleGenerator(3, seed=42)
    mic_invalid = None
    for cycle in range(1, 20):
        envelopes = microphone.next_cycle(start + timedelta(seconds=cycle), "mixed")
        telemetry = next(
            (item.payload for item in envelopes if item.payload.get("message_type") == "telemetry"),
            None,
        )
        if telemetry and not telemetry["mic_valid"]:
            mic_invalid = telemetry
            break
    assert mic_invalid is not None
    assert "acoustic_category" not in mic_invalid
    assert "acoustic_relative_level_dbfs" not in mic_invalid


def test_mixed_scenario_emits_offline_and_resumed_status() -> None:
    generator = VehicleGenerator(3, seed=42)
    start = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)
    states = []
    cycles_with_telemetry = []
    for cycle in range(1, 40):
        envelopes = generator.next_cycle(start + timedelta(seconds=cycle), "mixed")
        states.extend(
            item.payload["state"]
            for item in envelopes
            if item.payload.get("message_type") == "status"
        )
        if any(item.payload.get("message_type") == "telemetry" for item in envelopes):
            cycles_with_telemetry.append(cycle)
    assert states == ["offline", "online"]
    assert not set(range(18, 23)).intersection(cycles_with_telemetry)


def test_invalid_injection_is_explicit_and_rejected_by_schema() -> None:
    generator = VehicleGenerator(1, seed=42)
    envelopes = generator.next_cycle(
        datetime(2026, 7, 20, 23, 0, tzinfo=UTC),
        inject_invalid=True,
    )
    invalid = [item for item in envelopes if not item.validate]
    assert len(invalid) == 1
    with pytest.raises(ValueError, match="contract error"):
        generator.registry.validate(invalid[0].payload)


def test_command_acknowledgement_is_contract_valid() -> None:
    generator = VehicleGenerator(1, seed=42)
    now = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)
    command = {
        "schema_version": 1,
        "message_type": "command",
        "command_id": "cmd-simulator-0001",
        "device_id": generator.device_id,
        "vehicle_id": generator.vehicle_id,
        "command": "publish_status",
        "issued_at": "2026-07-20T22:59:59Z",
        "expires_at": "2026-07-20T23:10:00Z",
        "parameters": {},
        "requested_by": "test-operator",
    }
    generator.registry.validate(command)
    acknowledgement = generator.command_ack(command, now)
    generator.registry.validate(acknowledgement.payload)
    assert acknowledgement.payload["simulated"] is True
    assert acknowledgement.payload["state"] == "completed"


def test_expired_command_acknowledgement_is_contract_valid() -> None:
    generator = VehicleGenerator(1, seed=42)
    now = datetime(2026, 7, 20, 23, 0, tzinfo=UTC)
    command = {
        "schema_version": 1,
        "message_type": "command",
        "command_id": "cmd-simulator-expired",
        "device_id": generator.device_id,
        "vehicle_id": generator.vehicle_id,
        "command": "publish_status",
        "issued_at": "2026-07-20T22:00:00Z",
        "expires_at": "2026-07-20T22:01:00Z",
        "parameters": {},
        "requested_by": "test-operator",
    }
    acknowledgement = generator.command_ack(
        command,
        now,
        state="expired",
        message="Command expired before simulated execution",
    )
    generator.registry.validate(acknowledgement.payload)
    assert acknowledgement.payload["state"] == "expired"


def test_cli_dry_run_supports_multiple_vehicles_and_negative_injection(capsys) -> None:
    result = run(
        [
            "--dry-run",
            "--cycles",
            "2",
            "--vehicles",
            "2",
            "--interval",
            "0.1",
            "--inject-invalid-every",
            "2",
        ]
    )
    output = capsys.readouterr().out
    assert result == 0
    assert "sim-vehicle-001/sim-device-001 telemetry" in output
    assert "sim-vehicle-002/sim-device-002 acoustic" in output
    assert "invalid-injection" in output


def test_publisher_queue_is_bounded_and_marks_replayed_messages(monkeypatch) -> None:
    settings = SimulatorSettings(
        mqtt_host="example.invalid",
        mqtt_port=8883,
        mqtt_username="simulator",
        mqtt_password="not-a-real-secret",
        mqtt_ca_file=None,
        topic_prefix="vehiclesense/v1",
        vehicle_count=1,
        interval_seconds=1.0,
        seed=42,
        queue_limit=2,
    )
    generator = VehicleGenerator(1, registry=ContractRegistry())
    publisher = DevicePublisher(settings, generator)
    for sequence in range(1, 4):
        publisher._enqueue(
            Envelope(
                generator.branch_topic("telemetry"),
                {"message_type": "telemetry", "sequence": sequence, "replayed": False},
            )
        )

    assert publisher.queued == 2
    assert publisher.dropped == 1

    published: list[Envelope] = []
    monkeypatch.setattr(
        publisher,
        "_publish_now",
        lambda envelope: published.append(envelope) or True,
    )
    publisher.connected.set()

    assert publisher.flush() == 2
    assert publisher.queued == 0
    assert publisher.replayed == 2
    assert [item.payload["sequence"] for item in published] == [2, 3]
    assert all(item.payload["replayed"] is True for item in published)
