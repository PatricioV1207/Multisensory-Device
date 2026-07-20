#!/usr/bin/env python3
"""Validate all VehicleSense contract fixtures.

This is a development-time helper. It deliberately keeps `_contract` outside
the payload passed to JSON Schema so fixture routing metadata never appears on
MQTT.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

from jsonschema import Draft202012Validator, FormatChecker


ROOT = Path(__file__).resolve().parent
SCHEMAS = ROOT / "schemas"
FIXTURES = ROOT / "fixtures"


def load_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def validate_file(path: Path, should_be_valid: bool) -> list[str]:
    fixture = load_json(path)
    contract = fixture.pop("_contract", None)
    if not isinstance(contract, str):
        return [f"{path}: missing string _contract"]

    schema_path = SCHEMAS / contract
    if not schema_path.is_file():
        return [f"{path}: unknown contract {contract}"]

    validator = Draft202012Validator(
        load_json(schema_path), format_checker=FormatChecker()
    )
    errors = sorted(validator.iter_errors(fixture), key=lambda item: list(item.path))

    if should_be_valid and errors:
        details = "; ".join(error.message for error in errors)
        return [f"{path}: expected valid: {details}"]
    if not should_be_valid and not errors:
        return [f"{path}: expected rejection but schema accepted it"]
    return []


def main() -> int:
    failures: list[str] = []
    counts = {"valid": 0, "invalid": 0}
    for directory, expected in (("valid", True), ("invalid", False)):
        for path in sorted((FIXTURES / directory).glob("*.json")):
            counts[directory] += 1
            failures.extend(validate_file(path, expected))

    if failures:
        print("Contract validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1

    print(
        f"Contract fixtures passed: {counts['valid']} valid, "
        f"{counts['invalid']} invalid"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
