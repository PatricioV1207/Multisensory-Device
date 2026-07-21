from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator, FormatChecker

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
SCHEMA_DIRECTORY = REPOSITORY_ROOT / "contracts" / "schemas"

SCHEMA_BY_MESSAGE = {
    "telemetry": "telemetry-v3.schema.json",
    "status": "status-v1.schema.json",
    "acoustic": "acoustic-v1.schema.json",
    "event": "event-v1.schema.json",
    "command": "command-v1.schema.json",
    "command_ack": "command-ack-v1.schema.json",
}


class ContractRegistry:
    def __init__(self, schema_directory: Path = SCHEMA_DIRECTORY) -> None:
        self.validators = {
            message_type: Draft202012Validator(
                json.loads((schema_directory / filename).read_text(encoding="utf-8")),
                format_checker=FormatChecker(),
            )
            for message_type, filename in SCHEMA_BY_MESSAGE.items()
        }

    def validate(self, payload: dict[str, Any]) -> None:
        message_type = payload.get("message_type")
        validator = self.validators.get(message_type)
        if validator is None:
            raise ValueError(f"unsupported message_type: {message_type!r}")
        errors = sorted(validator.iter_errors(payload), key=lambda error: list(error.path))
        if errors:
            first = errors[0]
            location = ".".join(str(item) for item in first.path) or "root"
            raise ValueError(f"{message_type} contract error at {location}: {first.message}")


def topic(prefix: str, vehicle_id: str, device_id: str, branch: str) -> str:
    return f"{prefix.rstrip('/')}/vehicles/{vehicle_id}/devices/{device_id}/{branch}"
