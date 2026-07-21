from __future__ import annotations

from datetime import datetime
from typing import Any, Literal
from uuid import UUID

from pydantic import BaseModel, ConfigDict, Field

IDENTIFIER_PATTERN = r"^[A-Za-z0-9._-]+$"


class VehicleCreate(BaseModel):
    vehicle_id: str = Field(min_length=1, max_length=64, pattern=IDENTIFIER_PATTERN)
    display_name: str = Field(min_length=1, max_length=120)
    description: str | None = Field(default=None, max_length=2000)
    metadata: dict[str, Any] = Field(default_factory=dict)
    alert_thresholds: dict[str, Any] = Field(default_factory=dict)


class VehicleUpdate(BaseModel):
    display_name: str | None = Field(default=None, min_length=1, max_length=120)
    description: str | None = Field(default=None, max_length=2000)
    active: bool | None = None
    metadata: dict[str, Any] | None = None
    alert_thresholds: dict[str, Any] | None = None


class VehiclePublic(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: UUID
    vehicle_id: str
    display_name: str
    description: str | None
    active: bool
    metadata_json: dict[str, Any]
    alert_thresholds: dict[str, Any]


class DeviceCreate(BaseModel):
    device_id: str = Field(min_length=1, max_length=64, pattern=IDENTIFIER_PATTERN)
    vehicle_id: str = Field(min_length=1, max_length=64, pattern=IDENTIFIER_PATTERN)
    display_name: str | None = Field(default=None, max_length=120)
    simulated: bool = False


class DevicePublic(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: UUID
    device_id: str
    vehicle_id: UUID
    display_name: str | None
    firmware_version: str | None
    status: Literal["online", "offline", "stale", "unknown", "reconnecting"]
    status_reason: str | None
    last_seen_at: datetime | None
    simulated: bool


class AlertAction(BaseModel):
    action: Literal["acknowledge", "resolve"]


class CommandCreate(BaseModel):
    device_id: str = Field(min_length=1, max_length=64, pattern=IDENTIFIER_PATTERN)
    command_type: Literal[
        "publish_status",
        "publish_telemetry",
        "set_telemetry_interval",
        "set_acoustic_thresholds",
        "start_acoustic_calibration",
        "stop_acoustic_calibration",
    ]
    parameters: dict[str, Any] = Field(default_factory=dict)
    expires_in_seconds: int = Field(default=300, ge=10, le=86_400)


class Pagination(BaseModel):
    limit: int = Field(default=100, ge=1, le=1000)
    offset: int = Field(default=0, ge=0)
