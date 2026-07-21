from __future__ import annotations

import json

import pytest

from app.services.contract_validator import ContractValidationError, ContractValidator
from tests.conftest import contract_fixture, settings_for


def test_accepts_telemetry_and_enforces_topic_identity(tmp_path) -> None:
    validator = ContractValidator(settings_for("sqlite+aiosqlite:///:memory:"))
    payload = contract_fixture("telemetry_v3_full_simulated.json")
    topic = "vehiclesense/v1/vehicles/sim-vehicle-001/devices/sim-device-001/telemetry"
    message = validator.validate(topic, json.dumps(payload))
    assert message.message_type == "telemetry"
    assert message.payload["sample_id"] == "sim-device-001:7:42"

    wrong_topic = topic.replace("sim-device-001", "another-device")
    with pytest.raises(ContractValidationError, match="device_id"):
        validator.validate(wrong_topic, json.dumps(payload))


@pytest.mark.parametrize(
    "raw,error",
    [
        (b"{not-json", "valid UTF-8 JSON"),
        (b'{"schema_version":3,"temperature_c":NaN}', "numeric literal"),
        (b"[]", "root must be an object"),
    ],
)
def test_rejects_malformed_or_nonstandard_json(raw: bytes, error: str) -> None:
    validator = ContractValidator(settings_for("sqlite+aiosqlite:///:memory:"))
    topic = "vehiclesense/v1/vehicles/sim-vehicle-001/devices/sim-device-001/telemetry"
    with pytest.raises(ContractValidationError, match=error):
        validator.validate(topic, raw)


def test_rejects_unsupported_or_branch_mismatched_message() -> None:
    validator = ContractValidator(settings_for("sqlite+aiosqlite:///:memory:"))
    payload = contract_fixture("acoustic_v1_simulated.json")
    topic = "vehiclesense/v1/vehicles/sim-vehicle-001/devices/sim-device-001/events"
    with pytest.raises(ContractValidationError, match="message_type"):
        validator.validate(topic, json.dumps(payload))
