from __future__ import annotations

import math
import os
from dataclasses import dataclass
from pathlib import Path


def _integer(name: str, default: int, minimum: int, maximum: int) -> int:
    raw = os.getenv(name, str(default))
    try:
        value = int(raw)
    except ValueError as error:
        raise ValueError(f"{name} must be an integer") from error
    if value < minimum or value > maximum:
        raise ValueError(f"{name} must be between {minimum} and {maximum}")
    return value


def _floating(name: str, default: float, minimum: float, maximum: float) -> float:
    raw = os.getenv(name, str(default))
    try:
        value = float(raw)
    except ValueError as error:
        raise ValueError(f"{name} must be numeric") from error
    if not math.isfinite(value) or value < minimum or value > maximum:
        raise ValueError(f"{name} must be between {minimum} and {maximum}")
    return value


@dataclass(frozen=True, slots=True)
class SimulatorSettings:
    mqtt_host: str
    mqtt_port: int
    mqtt_username: str
    mqtt_password: str
    mqtt_ca_file: Path | None
    topic_prefix: str
    vehicle_count: int
    interval_seconds: float
    seed: int
    queue_limit: int

    @classmethod
    def from_environment(cls) -> SimulatorSettings:
        ca_value = os.getenv("VEHICLESENSE_SIM_MQTT_CA_FILE", "").strip()
        topic_prefix = (
            os.getenv("VEHICLESENSE_SIM_TOPIC_PREFIX", "vehiclesense/v1").strip().strip("/")
        )
        if not topic_prefix:
            topic_prefix = "vehiclesense/v1"
        if any(character in topic_prefix for character in ("#", "+", " ")):
            raise ValueError(
                "VEHICLESENSE_SIM_TOPIC_PREFIX cannot contain MQTT wildcards or spaces"
            )
        return cls(
            mqtt_host=os.getenv("VEHICLESENSE_SIM_MQTT_HOST", "").strip(),
            mqtt_port=_integer("VEHICLESENSE_SIM_MQTT_PORT", 8883, 1, 65_535),
            mqtt_username=os.getenv("VEHICLESENSE_SIM_MQTT_USERNAME", "").strip(),
            mqtt_password=os.getenv("VEHICLESENSE_SIM_MQTT_PASSWORD", ""),
            mqtt_ca_file=Path(ca_value) if ca_value else None,
            topic_prefix=topic_prefix,
            vehicle_count=_integer("VEHICLESENSE_SIM_VEHICLES", 3, 1, 100),
            interval_seconds=_floating("VEHICLESENSE_SIM_INTERVAL_SECONDS", 2.0, 0.1, 3600.0),
            seed=_integer("VEHICLESENSE_SIM_SEED", 42, 0, 2**31 - 1),
            queue_limit=_integer("VEHICLESENSE_SIM_QUEUE_LIMIT", 500, 1, 100_000),
        )

    def require_broker(self) -> None:
        missing = [
            name
            for name, value in {
                "VEHICLESENSE_SIM_MQTT_HOST": self.mqtt_host,
                "VEHICLESENSE_SIM_MQTT_USERNAME": self.mqtt_username,
                "VEHICLESENSE_SIM_MQTT_PASSWORD": self.mqtt_password,
            }.items()
            if not value
        ]
        if missing:
            raise ValueError(f"missing broker configuration: {', '.join(missing)}")
        if self.mqtt_ca_file is not None and not self.mqtt_ca_file.is_file():
            raise ValueError(f"MQTT CA file not found: {self.mqtt_ca_file}")
