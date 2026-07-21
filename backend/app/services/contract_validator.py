from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator, FormatChecker

from app.core.config import Settings

IDENTIFIER = r"[A-Za-z0-9._-]+"
BRANCH_TO_MESSAGE = {
    "telemetry": "telemetry",
    "status": "status",
    "events": "event",
    "acoustic": "acoustic",
    "command-acks": "command_ack",
}
SCHEMA_FILES = {
    ("telemetry", 2): "telemetry-v2.schema.json",
    ("telemetry", 3): "telemetry-v3.schema.json",
    ("status", 1): "status-v1.schema.json",
    ("event", 1): "event-v1.schema.json",
    ("acoustic", 1): "acoustic-v1.schema.json",
    ("command_ack", 1): "command-ack-v1.schema.json",
}


class ContractValidationError(ValueError):
    pass


@dataclass(frozen=True, slots=True)
class ValidatedMessage:
    branch: str
    message_type: str
    vehicle_id: str
    device_id: str
    payload: dict[str, Any]


def _reject_non_json_number(value: str):
    raise ContractValidationError(f"non-JSON numeric literal: {value}")


class ContractValidator:
    def __init__(self, settings: Settings) -> None:
        self.maximum_bytes = settings.maximum_mqtt_payload_bytes
        prefix = re.escape(settings.mqtt_topic_prefix.rstrip("/"))
        self.topic_pattern = re.compile(
            rf"^{prefix}/vehicles/(?P<vehicle>{IDENTIFIER})/devices/"
            rf"(?P<device>{IDENTIFIER})/(?P<branch>telemetry|status|events|acoustic|command-acks)$"
        )
        self.validators = self._load_validators(settings.contracts_directory)

    @staticmethod
    def _load_validators(directory: Path) -> dict[tuple[str, int], Draft202012Validator]:
        validators: dict[tuple[str, int], Draft202012Validator] = {}
        for key, filename in SCHEMA_FILES.items():
            path = directory / filename
            if not path.is_file():
                raise RuntimeError(f"missing contract schema: {path}")
            schema = json.loads(path.read_text(encoding="utf-8"))
            Draft202012Validator.check_schema(schema)
            validators[key] = Draft202012Validator(schema, format_checker=FormatChecker())
        return validators

    def validate(self, topic: str, raw_payload: bytes | str) -> ValidatedMessage:
        topic_match = self.topic_pattern.fullmatch(topic)
        if topic_match is None:
            raise ContractValidationError("topic does not match the VehicleSense namespace")
        encoded = raw_payload.encode("utf-8") if isinstance(raw_payload, str) else raw_payload
        if not encoded or len(encoded) > self.maximum_bytes:
            raise ContractValidationError("payload is empty or exceeds the configured limit")
        try:
            payload = json.loads(
                encoded.decode("utf-8"),
                parse_constant=_reject_non_json_number,
            )
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise ContractValidationError("payload is not valid UTF-8 JSON") from error
        if not isinstance(payload, dict):
            raise ContractValidationError("payload root must be an object")

        branch = topic_match.group("branch")
        expected_type = BRANCH_TO_MESSAGE[branch]
        version = payload.get("schema_version")
        message_type = payload.get("message_type", "telemetry" if version == 2 else None)
        if message_type != expected_type:
            raise ContractValidationError("topic branch and message_type do not agree")
        validator = self.validators.get((message_type, version))
        if validator is None:
            raise ContractValidationError("unsupported message type or schema version")
        errors = sorted(validator.iter_errors(payload), key=lambda item: list(item.path))
        if errors:
            first = errors[0]
            location = ".".join(str(part) for part in first.absolute_path) or "$"
            raise ContractValidationError(f"contract violation at {location}: {first.message}")

        topic_vehicle = topic_match.group("vehicle")
        topic_device = topic_match.group("device")
        if payload.get("device_id") != topic_device:
            raise ContractValidationError("topic device_id does not match payload")
        if version != 2 and payload.get("vehicle_id") != topic_vehicle:
            raise ContractValidationError("topic vehicle_id does not match payload")
        return ValidatedMessage(
            branch=branch,
            message_type=message_type,
            vehicle_id=topic_vehicle,
            device_id=topic_device,
            payload=payload,
        )
