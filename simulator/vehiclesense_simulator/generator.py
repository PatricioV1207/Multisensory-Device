from __future__ import annotations

import copy
import math
import random
from dataclasses import dataclass
from datetime import UTC, datetime
from typing import Any

from vehiclesense_simulator.contracts import ContractRegistry, topic

ACOUSTIC_CATEGORIES = ("quiet", "engine", "traffic", "speech", "music", "unknown")


def iso(value: datetime) -> str:
    return value.astimezone(UTC).isoformat(timespec="seconds").replace("+00:00", "Z")


@dataclass(frozen=True, slots=True)
class Envelope:
    topic: str
    payload: dict[str, Any]
    qos: int = 1
    retain: bool = False
    validate: bool = True


class VehicleGenerator:
    def __init__(
        self,
        index: int,
        topic_prefix: str = "vehiclesense/v1",
        seed: int = 42,
        registry: ContractRegistry | None = None,
    ) -> None:
        self.index = index
        self.vehicle_id = f"sim-vehicle-{index:03d}"
        self.device_id = f"sim-device-{index:03d}"
        self.topic_prefix = topic_prefix.rstrip("/")
        self.random = random.Random(seed + index * 1009)
        self.registry = registry or ContractRegistry()
        self.boot_id = (seed * 1000 + index) % (2**32)
        self.sequence = 0
        self.cycle = 0
        self.started_at: datetime | None = None
        self.offline = False

    def branch_topic(self, branch: str) -> str:
        return topic(self.topic_prefix, self.vehicle_id, self.device_id, branch)

    def status(self, state: str, reason: str, now: datetime) -> Envelope:
        uptime_ms = self._uptime(now)
        payload: dict[str, Any] = {
            "schema_version": 1,
            "message_type": "status",
            "device_id": self.device_id,
            "vehicle_id": self.vehicle_id,
            "boot_id": self.boot_id,
            "state": state,
            "reason": reason,
            "firmware_version": "simulator-0.1.0",
            "uptime_ms": uptime_ms,
            "time_valid": True,
            "observed_at": iso(now),
            "simulated": True,
        }
        if state != "offline":
            payload.update(wifi_rssi_dbm=-48 - self.index * 3, ip_address=f"192.0.2.{self.index}")
        self.registry.validate(payload)
        return Envelope(self.branch_topic("status"), payload, retain=True)

    def next_cycle(
        self,
        now: datetime,
        scenario: str = "mixed",
        inject_invalid: bool = False,
    ) -> list[Envelope]:
        if self.started_at is None:
            self.started_at = now
        self.cycle += 1
        outage = scenario == "mixed" and self.index == 3 and 18 <= self.cycle % 35 <= 22
        envelopes: list[Envelope] = []
        if outage and not self.offline:
            self.offline = True
            envelopes.append(self.status("offline", "network_lost", now))
        elif not outage and self.offline:
            self.offline = False
            envelopes.append(self.status("online", "connected", now))
        if outage:
            return envelopes

        self.sequence += 1
        telemetry = self._telemetry(now, scenario)
        acoustic = self._acoustic(now, telemetry)
        envelopes.extend(
            [
                Envelope(self.branch_topic("telemetry"), telemetry),
                Envelope(self.branch_topic("acoustic"), acoustic),
            ]
        )
        event = self._event(now, telemetry, scenario)
        if event is not None:
            envelopes.append(Envelope(self.branch_topic("events"), event))
        if inject_invalid:
            invalid = copy.deepcopy(telemetry)
            invalid["unexpected_negative_test_field"] = True
            envelopes.append(
                Envelope(
                    self.branch_topic("telemetry"),
                    invalid,
                    validate=False,
                )
            )
        for envelope in envelopes:
            if envelope.validate:
                self.registry.validate(envelope.payload)
        return envelopes

    def command_ack(
        self,
        command: dict[str, Any],
        now: datetime,
        state: str = "completed",
        message: str | None = None,
    ) -> Envelope:
        payload = {
            "schema_version": 1,
            "message_type": "command_ack",
            "command_id": command["command_id"],
            "device_id": self.device_id,
            "vehicle_id": self.vehicle_id,
            "state": state,
            "uptime_ms": self._uptime(now),
            "time_valid": True,
            "acknowledged_at": iso(now),
            "message": message or f"Simulated command {command['command']} {state}",
            "result": {},
            "simulated": True,
        }
        self.registry.validate(payload)
        return Envelope(self.branch_topic("command-acks"), payload)

    def _uptime(self, now: datetime) -> int:
        start = self.started_at or now
        return max(0, int((now - start).total_seconds() * 1000))

    def _motion(self, scenario: str) -> tuple[float, float, float]:
        phase = self.cycle * 0.085 + self.index * 0.7
        stopped = scenario == "mixed" and (
            (self.index == 2 and 10 <= self.cycle % 28 <= 16)
            or (self.index == 1 and 45 <= self.cycle % 70 <= 50)
        )
        speed = 0.0 if stopped else max(4.0, 31 + 13 * math.sin(phase * 1.7))
        latitude = -2.1709 + self.index * 0.004 + 0.021 * math.sin(phase)
        longitude = -79.9224 + self.index * 0.003 + 0.028 * math.sin(phase * 0.73)
        return latitude, longitude, speed

    def _telemetry(self, now: datetime, scenario: str) -> dict[str, Any]:
        latitude, longitude, speed = self._motion(scenario)
        phase = self.cycle * 0.085 + self.index
        gps_valid = not (scenario == "mixed" and self.index == 2 and 22 <= self.cycle % 40 <= 25)
        microphone_valid = not (
            scenario == "mixed" and self.index == 3 and 8 <= self.cycle % 32 <= 10
        )
        temperature = 24.0 + self.index + 3.2 * math.sin(phase / 3)
        if scenario == "stress" and self.index == 1:
            temperature = 43.0
        category = ACOUSTIC_CATEGORIES[(self.cycle // 5 + self.index) % len(ACOUSTIC_CATEGORIES)]
        acoustic_valid = microphone_valid
        uptime = self._uptime(now)
        payload: dict[str, Any] = {
            "schema_version": 3,
            "message_type": "telemetry",
            "device_id": self.device_id,
            "vehicle_id": self.vehicle_id,
            "boot_id": self.boot_id,
            "sequence": self.sequence,
            "sample_id": f"{self.device_id}:{self.boot_id}:{self.sequence}",
            "uptime_ms": uptime,
            "time_valid": True,
            "measured_at": iso(now),
            "replayed": False,
            "simulated": True,
            "dht_valid": True,
            "temperature_c": round(temperature, 2),
            "humidity_percent": round(58 + 12 * math.cos(phase / 4), 2),
            "gps_valid": gps_valid,
            "accel_valid": True,
            "accel_x": round(0.32 * math.sin(phase * 2), 4),
            "accel_y": round(0.18 * math.cos(phase * 1.3), 4),
            "accel_z": round(9.80665 + 0.12 * math.sin(phase * 3.2), 4),
            "gyro_valid": True,
            "gyro_x": round(0.02 * math.sin(phase), 5),
            "gyro_y": round(0.03 * math.cos(phase), 5),
            "gyro_z": round(0.05 * math.sin(phase / 2), 5),
            "mag_valid": True,
            "mag_x": round(24 + 4 * math.sin(phase), 3),
            "mag_y": round(-11 + 3 * math.cos(phase), 3),
            "mag_z": round(47 + 2 * math.sin(phase / 2), 3),
            "imu_valid": True,
            "baro_valid": True,
            "pressure_hpa": round(1006.7 + 1.1 * math.sin(phase / 8), 2),
            "sea_level_pressure_hpa": 1008.0,
            "baro_temperature_c": round(temperature + 0.6, 2),
            "baro_altitude_m": round(10.0 + 3 * math.sin(phase / 8), 2),
            "bh1750_valid": True,
            "light_lux": round(max(0.0, 320 + 250 * math.sin(phase / 5)), 1),
            "sd_valid": True,
            "offline_queued": 0,
            "offline_replayed": 0,
            "offline_dropped": 0,
            "offline_oldest_age_s": 0,
            "wifi_connected": True,
            "wifi_rssi_dbm": -48 - self.index * 3,
            "mqtt_connected": True,
            "mic_valid": microphone_valid,
            "acoustic_valid": acoustic_valid,
        }
        if gps_valid:
            payload.update(
                latitude=round(latitude, 7),
                longitude=round(longitude, 7),
                altitude_m=round(9.5 + 2 * math.sin(phase), 2),
                speed_kmh=round(speed, 2),
                satellites=9 + self.index,
                gps_hdop=round(0.8 + self.index * 0.12, 2),
            )
        if acoustic_valid:
            level = -46 + 16 * (0.5 + 0.5 * math.sin(phase * 1.4))
            confidence = 0.48 if category == "unknown" else 0.72 + self.index * 0.04
            payload.update(
                acoustic_relative_level_dbfs=round(level, 2),
                acoustic_peak_dbfs=round(min(-1.0, level + 13), 2),
                acoustic_category=category,
                acoustic_confidence=round(min(confidence, 0.96), 2),
                acoustic_clipping=False,
                acoustic_classifier_version="sim-heuristic-1",
            )
        return payload

    def _acoustic(self, now: datetime, telemetry: dict[str, Any]) -> dict[str, Any]:
        valid = bool(telemetry["acoustic_valid"])
        payload: dict[str, Any] = {
            "schema_version": 1,
            "message_type": "acoustic",
            "device_id": self.device_id,
            "vehicle_id": self.vehicle_id,
            "boot_id": self.boot_id,
            "sequence": self.sequence,
            "sample_id": f"{telemetry['sample_id']}:acoustic",
            "uptime_ms": telemetry["uptime_ms"],
            "time_valid": True,
            "measured_at": iso(now),
            "replayed": False,
            "simulated": True,
            "mic_valid": telemetry["mic_valid"],
            "analysis_valid": valid,
        }
        if valid:
            payload.update(
                window_duration_ms=1024,
                sample_rate_hz=16000,
                relative_level_dbfs=telemetry["acoustic_relative_level_dbfs"],
                peak_dbfs=telemetry["acoustic_peak_dbfs"],
                clipping=telemetry["acoustic_clipping"],
                clipping_ratio=0.0,
                crest_factor=3.1,
                zero_crossing_rate=0.09,
                spectral_centroid_hz=1450.0,
                spectral_flatness=0.38,
                spectral_rolloff_hz=3200.0,
                band_energy_ratio={
                    "hz_80_250": 0.18,
                    "hz_250_800": 0.28,
                    "hz_800_2000": 0.31,
                    "hz_2000_4000": 0.16,
                    "hz_4000_8000": 0.07,
                },
                category=telemetry["acoustic_category"],
                confidence=telemetry["acoustic_confidence"],
                classifier_version="sim-heuristic-1",
            )
        return payload

    def _event(
        self, now: datetime, telemetry: dict[str, Any], scenario: str
    ) -> dict[str, Any] | None:
        event_type = None
        severity = "medium"
        details: dict[str, Any] = {}
        if self.cycle % 30 == 0:
            event_type = "acoustic_horn"
            severity = "high"
            details = {"category": "horn", "source": "simulated_scenario"}
        elif scenario == "stress" and self.index == 1 and self.cycle % 10 == 0:
            event_type = "temperature_high"
            severity = "high"
            details = {"temperature_c": telemetry["temperature_c"]}
        if event_type is None:
            return None
        return {
            "schema_version": 1,
            "message_type": "event",
            "event_id": f"{self.device_id}:{self.boot_id}:event:{self.cycle}",
            "device_id": self.device_id,
            "vehicle_id": self.vehicle_id,
            "event_type": event_type,
            "severity": severity,
            "uptime_ms": telemetry["uptime_ms"],
            "time_valid": True,
            "occurred_at": iso(now),
            "details": details,
            "simulated": True,
        }
