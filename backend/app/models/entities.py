from __future__ import annotations

from datetime import datetime
from typing import Any
from uuid import UUID, uuid4

from sqlalchemy import (
    JSON,
    BigInteger,
    Boolean,
    DateTime,
    Float,
    ForeignKey,
    Index,
    Integer,
    String,
    Text,
)
from sqlalchemy.orm import Mapped, mapped_column, relationship
from sqlalchemy.sql import func
from sqlalchemy.types import Uuid

from app.db.base import Base, CreatedAtMixin


class User(CreatedAtMixin, Base):
    __tablename__ = "users"

    id: Mapped[UUID] = mapped_column(Uuid(as_uuid=True), primary_key=True, default=uuid4)
    email: Mapped[str] = mapped_column(String(320), unique=True, index=True)
    password_hash: Mapped[str] = mapped_column(String(255))
    role: Mapped[str] = mapped_column(String(20), default="viewer", index=True)
    active: Mapped[bool] = mapped_column(Boolean, default=True)
    last_login_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))


class Vehicle(CreatedAtMixin, Base):
    __tablename__ = "vehicles"

    id: Mapped[UUID] = mapped_column(Uuid(as_uuid=True), primary_key=True, default=uuid4)
    vehicle_id: Mapped[str] = mapped_column(String(64), unique=True, index=True)
    display_name: Mapped[str] = mapped_column(String(120))
    description: Mapped[str | None] = mapped_column(Text)
    active: Mapped[bool] = mapped_column(Boolean, default=True, index=True)
    metadata_json: Mapped[dict[str, Any]] = mapped_column(JSON, default=dict)
    alert_thresholds: Mapped[dict[str, Any]] = mapped_column(JSON, default=dict)

    devices: Mapped[list[Device]] = relationship(back_populates="vehicle")
    telemetry: Mapped[list[Telemetry]] = relationship(back_populates="vehicle")
    trips: Mapped[list[Trip]] = relationship(back_populates="vehicle")


class Device(CreatedAtMixin, Base):
    __tablename__ = "devices"

    id: Mapped[UUID] = mapped_column(Uuid(as_uuid=True), primary_key=True, default=uuid4)
    device_id: Mapped[str] = mapped_column(String(64), unique=True, index=True)
    vehicle_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("vehicles.id", ondelete="CASCADE"), index=True
    )
    display_name: Mapped[str | None] = mapped_column(String(120))
    firmware_version: Mapped[str | None] = mapped_column(String(64))
    status: Mapped[str] = mapped_column(String(20), default="unknown", index=True)
    status_reason: Mapped[str | None] = mapped_column(String(120))
    last_seen_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), index=True)
    last_status_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    last_boot_id: Mapped[int | None] = mapped_column(BigInteger)
    simulated: Mapped[bool] = mapped_column(Boolean, default=False, index=True)
    motion_candidate_since: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    stopped_candidate_since: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    gps_invalid_since: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    microphone_invalid_since: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))

    vehicle: Mapped[Vehicle] = relationship(back_populates="devices")


class Telemetry(CreatedAtMixin, Base):
    __tablename__ = "telemetry"
    __table_args__ = (
        Index("ix_telemetry_vehicle_received", "vehicle_id", "received_at"),
        Index("ix_telemetry_device_received", "device_id", "received_at"),
    )

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    sample_id: Mapped[str | None] = mapped_column(String(170), unique=True)
    dedupe_key: Mapped[str] = mapped_column(String(200), unique=True)
    schema_version: Mapped[int] = mapped_column(Integer)
    legacy: Mapped[bool] = mapped_column(Boolean, default=False)
    device_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("devices.id", ondelete="CASCADE"), index=True
    )
    vehicle_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("vehicles.id", ondelete="CASCADE"), index=True
    )
    boot_id: Mapped[int | None] = mapped_column(BigInteger)
    sequence: Mapped[int | None] = mapped_column(BigInteger)
    measured_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), index=True)
    received_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now(), index=True
    )
    uptime_ms: Mapped[int] = mapped_column(BigInteger)
    replayed: Mapped[bool] = mapped_column(Boolean, default=False)
    simulated: Mapped[bool] = mapped_column(Boolean, default=False, index=True)

    dht_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    gps_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    accel_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    gyro_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    mag_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    baro_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    imu_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    light_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    storage_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    microphone_valid: Mapped[bool] = mapped_column(Boolean, default=False)
    acoustic_valid: Mapped[bool] = mapped_column(Boolean, default=False)

    temperature_c: Mapped[float | None] = mapped_column(Float)
    humidity_percent: Mapped[float | None] = mapped_column(Float)
    latitude: Mapped[float | None] = mapped_column(Float)
    longitude: Mapped[float | None] = mapped_column(Float)
    gps_altitude_m: Mapped[float | None] = mapped_column(Float)
    speed_kmh: Mapped[float | None] = mapped_column(Float)
    satellites: Mapped[int | None] = mapped_column(Integer)
    gps_hdop: Mapped[float | None] = mapped_column(Float)
    pressure_hpa: Mapped[float | None] = mapped_column(Float)
    baro_altitude_m: Mapped[float | None] = mapped_column(Float)
    light_lux: Mapped[float | None] = mapped_column(Float)
    accel_x: Mapped[float | None] = mapped_column(Float)
    accel_y: Mapped[float | None] = mapped_column(Float)
    accel_z: Mapped[float | None] = mapped_column(Float)
    gyro_x: Mapped[float | None] = mapped_column(Float)
    gyro_y: Mapped[float | None] = mapped_column(Float)
    gyro_z: Mapped[float | None] = mapped_column(Float)
    mag_x: Mapped[float | None] = mapped_column(Float)
    mag_y: Mapped[float | None] = mapped_column(Float)
    mag_z: Mapped[float | None] = mapped_column(Float)
    acoustic_level_dbfs: Mapped[float | None] = mapped_column(Float)
    acoustic_category: Mapped[str | None] = mapped_column(String(24))
    acoustic_confidence: Mapped[float | None] = mapped_column(Float)
    raw_payload: Mapped[dict[str, Any]] = mapped_column(JSON)

    vehicle: Mapped[Vehicle] = relationship(back_populates="telemetry")
    device: Mapped[Device] = relationship()


class AcousticMeasurement(CreatedAtMixin, Base):
    __tablename__ = "acoustic_measurements"
    __table_args__ = (Index("ix_acoustic_vehicle_received", "vehicle_id", "received_at"),)

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    sample_id: Mapped[str] = mapped_column(String(170), unique=True)
    device_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("devices.id", ondelete="CASCADE"), index=True
    )
    vehicle_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("vehicles.id", ondelete="CASCADE"), index=True
    )
    measured_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    received_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now()
    )
    microphone_valid: Mapped[bool] = mapped_column(Boolean)
    analysis_valid: Mapped[bool] = mapped_column(Boolean)
    relative_level_dbfs: Mapped[float | None] = mapped_column(Float)
    peak_dbfs: Mapped[float | None] = mapped_column(Float)
    clipping: Mapped[bool | None] = mapped_column(Boolean)
    category: Mapped[str | None] = mapped_column(String(24))
    confidence: Mapped[float | None] = mapped_column(Float)
    classifier_version: Mapped[str | None] = mapped_column(String(40))
    features: Mapped[dict[str, Any]] = mapped_column(JSON, default=dict)
    simulated: Mapped[bool] = mapped_column(Boolean, default=False)
    raw_payload: Mapped[dict[str, Any]] = mapped_column(JSON)


class Alert(CreatedAtMixin, Base):
    __tablename__ = "alerts"
    __table_args__ = (
        Index("ix_alert_vehicle_status_triggered", "vehicle_id", "status", "triggered_at"),
    )

    id: Mapped[UUID] = mapped_column(Uuid(as_uuid=True), primary_key=True, default=uuid4)
    event_key: Mapped[str] = mapped_column(String(255), unique=True)
    external_event_id: Mapped[str | None] = mapped_column(String(170), unique=True)
    vehicle_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("vehicles.id", ondelete="CASCADE"), index=True
    )
    device_id: Mapped[UUID | None] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("devices.id", ondelete="SET NULL"), index=True
    )
    trip_id: Mapped[UUID | None] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("trips.id", ondelete="SET NULL"), index=True
    )
    alert_type: Mapped[str] = mapped_column(String(64), index=True)
    severity: Mapped[str] = mapped_column(String(16), index=True)
    status: Mapped[str] = mapped_column(String(16), default="active", index=True)
    source: Mapped[str] = mapped_column(String(32), default="backend")
    triggered_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), index=True)
    last_observed_at: Mapped[datetime] = mapped_column(DateTime(timezone=True))
    acknowledged_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    acknowledged_by: Mapped[UUID | None] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("users.id", ondelete="SET NULL")
    )
    resolved_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    occurrence_count: Mapped[int] = mapped_column(Integer, default=1)
    title: Mapped[str] = mapped_column(String(160))
    details: Mapped[dict[str, Any]] = mapped_column(JSON, default=dict)
    simulated: Mapped[bool] = mapped_column(Boolean, default=False)


class Trip(CreatedAtMixin, Base):
    __tablename__ = "trips"
    __table_args__ = (Index("ix_trip_vehicle_start", "vehicle_id", "started_at"),)

    id: Mapped[UUID] = mapped_column(Uuid(as_uuid=True), primary_key=True, default=uuid4)
    vehicle_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("vehicles.id", ondelete="CASCADE"), index=True
    )
    device_id: Mapped[UUID | None] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("devices.id", ondelete="SET NULL"), index=True
    )
    status: Mapped[str] = mapped_column(String(16), default="active", index=True)
    started_at: Mapped[datetime] = mapped_column(DateTime(timezone=True))
    ended_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    start_latitude: Mapped[float | None] = mapped_column(Float)
    start_longitude: Mapped[float | None] = mapped_column(Float)
    end_latitude: Mapped[float | None] = mapped_column(Float)
    end_longitude: Mapped[float | None] = mapped_column(Float)
    distance_km: Mapped[float] = mapped_column(Float, default=0.0)
    average_speed_kmh: Mapped[float | None] = mapped_column(Float)
    maximum_speed_kmh: Mapped[float | None] = mapped_column(Float)
    speed_sum_kmh: Mapped[float] = mapped_column(Float, default=0.0)
    speed_samples: Mapped[int] = mapped_column(Integer, default=0)
    point_count: Mapped[int] = mapped_column(Integer, default=0)
    last_latitude: Mapped[float | None] = mapped_column(Float)
    last_longitude: Mapped[float | None] = mapped_column(Float)
    last_point_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    simulated: Mapped[bool] = mapped_column(Boolean, default=False)

    vehicle: Mapped[Vehicle] = relationship(back_populates="trips")
    points: Mapped[list[TripPoint]] = relationship(
        back_populates="trip", cascade="all, delete-orphan"
    )


class TripPoint(Base):
    __tablename__ = "trip_points"
    __table_args__ = (Index("ix_trip_point_trip_time", "trip_id", "recorded_at"),)

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    trip_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("trips.id", ondelete="CASCADE"), index=True
    )
    telemetry_id: Mapped[int | None] = mapped_column(
        Integer, ForeignKey("telemetry.id", ondelete="SET NULL"), unique=True
    )
    recorded_at: Mapped[datetime] = mapped_column(DateTime(timezone=True))
    latitude: Mapped[float] = mapped_column(Float)
    longitude: Mapped[float] = mapped_column(Float)
    speed_kmh: Mapped[float | None] = mapped_column(Float)
    hdop: Mapped[float | None] = mapped_column(Float)

    trip: Mapped[Trip] = relationship(back_populates="points")


class DeviceStatusHistory(Base):
    __tablename__ = "device_status_history"
    __table_args__ = (Index("ix_status_device_changed", "device_id", "changed_at"),)

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("devices.id", ondelete="CASCADE"), index=True
    )
    state: Mapped[str] = mapped_column(String(20))
    reason: Mapped[str | None] = mapped_column(String(120))
    changed_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())
    payload: Mapped[dict[str, Any]] = mapped_column(JSON, default=dict)


class Command(CreatedAtMixin, Base):
    __tablename__ = "commands"

    id: Mapped[UUID] = mapped_column(Uuid(as_uuid=True), primary_key=True, default=uuid4)
    command_id: Mapped[str] = mapped_column(String(100), unique=True, index=True)
    device_id: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("devices.id", ondelete="CASCADE"), index=True
    )
    command_type: Mapped[str] = mapped_column(String(64))
    state: Mapped[str] = mapped_column(String(24), default="queued", index=True)
    requested_by: Mapped[UUID] = mapped_column(
        Uuid(as_uuid=True), ForeignKey("users.id", ondelete="RESTRICT")
    )
    expires_at: Mapped[datetime] = mapped_column(DateTime(timezone=True))
    published_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    acknowledged_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    parameters: Mapped[dict[str, Any]] = mapped_column(JSON, default=dict)
    acknowledgement: Mapped[dict[str, Any] | None] = mapped_column(JSON)


class IngestionFailure(Base):
    __tablename__ = "ingestion_failures"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    topic: Mapped[str] = mapped_column(String(512), index=True)
    payload_preview: Mapped[str] = mapped_column(Text)
    error: Mapped[str] = mapped_column(Text)
    received_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), index=True
    )
