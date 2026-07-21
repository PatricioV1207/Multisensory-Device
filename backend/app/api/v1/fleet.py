from __future__ import annotations

from datetime import UTC, datetime, timedelta
from typing import Any
from uuid import UUID

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import func, select
from sqlalchemy.ext.asyncio import AsyncSession

from app.api.dependencies import current_user, get_session, require_roles
from app.models import (
    AcousticMeasurement,
    Alert,
    Device,
    Telemetry,
    Trip,
    TripPoint,
    User,
    Vehicle,
)
from app.schemas.fleet import AlertAction, DeviceCreate, VehicleCreate, VehicleUpdate

router = APIRouter(tags=["fleet"], dependencies=[Depends(current_user)])


def iso(value: datetime | None) -> str | None:
    return value.isoformat() if value else None


def telemetry_data(item: Telemetry | None) -> dict[str, Any] | None:
    if item is None:
        return None
    return {
        "id": item.id,
        "sample_id": item.sample_id,
        "schema_version": item.schema_version,
        "legacy": item.legacy,
        "measured_at": iso(item.measured_at),
        "received_at": iso(item.received_at),
        "replayed": item.replayed,
        "simulated": item.simulated,
        "validity": {
            "dht": item.dht_valid,
            "gps": item.gps_valid,
            "accel": item.accel_valid,
            "gyro": item.gyro_valid,
            "mag": item.mag_valid,
            "barometer": item.baro_valid,
            "imu": item.imu_valid,
            "light": item.light_valid,
            "storage": item.storage_valid,
            "microphone": item.microphone_valid,
            "acoustic": item.acoustic_valid,
        },
        "temperature_c": item.temperature_c,
        "humidity_percent": item.humidity_percent,
        "latitude": item.latitude,
        "longitude": item.longitude,
        "gps_altitude_m": item.gps_altitude_m,
        "speed_kmh": item.speed_kmh,
        "satellites": item.satellites,
        "gps_hdop": item.gps_hdop,
        "pressure_hpa": item.pressure_hpa,
        "baro_altitude_m": item.baro_altitude_m,
        "light_lux": item.light_lux,
        "acceleration_mps2": [item.accel_x, item.accel_y, item.accel_z]
        if item.accel_valid
        else None,
        "gyroscope_rad_s": [item.gyro_x, item.gyro_y, item.gyro_z] if item.gyro_valid else None,
        "magnetometer_ut": [item.mag_x, item.mag_y, item.mag_z] if item.mag_valid else None,
        "acoustic": {
            "relative_level_dbfs": item.acoustic_level_dbfs,
            "category": item.acoustic_category,
            "confidence": item.acoustic_confidence,
        }
        if item.acoustic_valid
        else None,
    }


def alert_data(item: Alert, vehicle_external_id: str | None = None) -> dict[str, Any]:
    return {
        "id": str(item.id),
        "vehicle_id": vehicle_external_id,
        "device_id": str(item.device_id) if item.device_id else None,
        "trip_id": str(item.trip_id) if item.trip_id else None,
        "type": item.alert_type,
        "severity": item.severity,
        "status": item.status,
        "title": item.title,
        "triggered_at": iso(item.triggered_at),
        "last_observed_at": iso(item.last_observed_at),
        "acknowledged_at": iso(item.acknowledged_at),
        "resolved_at": iso(item.resolved_at),
        "occurrence_count": item.occurrence_count,
        "details": item.details,
        "simulated": item.simulated,
    }


def trip_data(item: Trip, vehicle_external_id: str | None = None) -> dict[str, Any]:
    duration = None
    if item.ended_at:
        duration = max(0, int((item.ended_at - item.started_at).total_seconds()))
    return {
        "id": str(item.id),
        "vehicle_id": vehicle_external_id,
        "status": item.status,
        "started_at": iso(item.started_at),
        "ended_at": iso(item.ended_at),
        "duration_seconds": duration,
        "distance_km": item.distance_km,
        "average_speed_kmh": item.average_speed_kmh,
        "maximum_speed_kmh": item.maximum_speed_kmh,
        "point_count": item.point_count,
        "start": [item.start_latitude, item.start_longitude]
        if item.start_latitude is not None and item.start_longitude is not None
        else None,
        "end": [item.end_latitude, item.end_longitude]
        if item.end_latitude is not None and item.end_longitude is not None
        else None,
        "simulated": item.simulated,
    }


async def latest_for_vehicle(session: AsyncSession, vehicle: Vehicle) -> Telemetry | None:
    return await session.scalar(
        select(Telemetry)
        .where(Telemetry.vehicle_id == vehicle.id)
        .order_by(Telemetry.received_at.desc())
        .limit(1)
    )


@router.get("/dashboard")
async def dashboard(session: AsyncSession = Depends(get_session)) -> dict[str, Any]:
    vehicles = list((await session.scalars(select(Vehicle).where(Vehicle.active))).all())
    summaries: list[dict[str, Any]] = []
    positions: list[dict[str, Any]] = []
    online = in_motion = stopped = 0
    for vehicle in vehicles:
        devices = list(
            (await session.scalars(select(Device).where(Device.vehicle_id == vehicle.id))).all()
        )
        latest = await latest_for_vehicle(session, vehicle)
        states = {device.status for device in devices}
        state = (
            "online"
            if "online" in states
            else "stale"
            if "stale" in states
            else "offline"
            if "offline" in states
            else "unknown"
        )
        moving = bool(latest and latest.gps_valid and (latest.speed_kmh or 0.0) >= 5.0)
        if state == "online":
            online += 1
            if moving:
                in_motion += 1
            else:
                stopped += 1
        summary = {
            "vehicle_id": vehicle.vehicle_id,
            "display_name": vehicle.display_name,
            "status": state,
            "motion_state": "in_motion" if moving else "stopped" if state == "online" else state,
            "last_update": iso(latest.received_at) if latest else None,
            "latest_telemetry": telemetry_data(latest),
            "devices": [device.device_id for device in devices],
        }
        summaries.append(summary)
        if (
            latest
            and latest.gps_valid
            and latest.latitude is not None
            and latest.longitude is not None
        ):
            positions.append(
                {
                    "vehicle_id": vehicle.vehicle_id,
                    "display_name": vehicle.display_name,
                    "latitude": latest.latitude,
                    "longitude": latest.longitude,
                    "speed_kmh": latest.speed_kmh,
                    "status": state,
                    "received_at": iso(latest.received_at),
                }
            )

    active_alerts = await session.scalar(
        select(func.count(Alert.id)).where(Alert.status.in_(["active", "acknowledged"]))
    )
    recent_alerts = list(
        (await session.scalars(select(Alert).order_by(Alert.triggered_at.desc()).limit(8))).all()
    )
    recent_trips = list(
        (await session.scalars(select(Trip).order_by(Trip.started_at.desc()).limit(8))).all()
    )
    vehicle_ids = {vehicle.id: vehicle.vehicle_id for vehicle in vehicles}
    return {
        "generated_at": datetime.now(UTC).isoformat(),
        "totals": {
            "vehicles": len(vehicles),
            "online": online,
            "in_motion": in_motion,
            "stopped": stopped,
            "active_alerts": active_alerts or 0,
        },
        "vehicles": summaries,
        "positions": positions,
        "recent_alerts": [
            alert_data(item, vehicle_ids.get(item.vehicle_id)) for item in recent_alerts
        ],
        "recent_trips": [
            trip_data(item, vehicle_ids.get(item.vehicle_id)) for item in recent_trips
        ],
    }


@router.get("/vehicles")
async def list_vehicles(session: AsyncSession = Depends(get_session)) -> list[dict[str, Any]]:
    vehicles = list((await session.scalars(select(Vehicle).order_by(Vehicle.display_name))).all())
    result = []
    for vehicle in vehicles:
        latest = await latest_for_vehicle(session, vehicle)
        devices = list(
            (await session.scalars(select(Device).where(Device.vehicle_id == vehicle.id))).all()
        )
        result.append(
            {
                "vehicle_id": vehicle.vehicle_id,
                "display_name": vehicle.display_name,
                "description": vehicle.description,
                "active": vehicle.active,
                "metadata": vehicle.metadata_json,
                "alert_thresholds": vehicle.alert_thresholds,
                "devices": [
                    {
                        "device_id": item.device_id,
                        "status": item.status,
                        "last_seen_at": iso(item.last_seen_at),
                        "firmware_version": item.firmware_version,
                        "simulated": item.simulated,
                    }
                    for item in devices
                ],
                "latest_telemetry": telemetry_data(latest),
            }
        )
    return result


@router.post("/vehicles", status_code=201)
async def create_vehicle(
    request: VehicleCreate,
    session: AsyncSession = Depends(get_session),
    _: User = Depends(require_roles("admin")),
) -> dict[str, Any]:
    if await session.scalar(select(Vehicle.id).where(Vehicle.vehicle_id == request.vehicle_id)):
        raise HTTPException(status_code=409, detail="vehicle_id already exists")
    vehicle = Vehicle(
        vehicle_id=request.vehicle_id,
        display_name=request.display_name,
        description=request.description,
        metadata_json=request.metadata,
        alert_thresholds=request.alert_thresholds,
    )
    session.add(vehicle)
    await session.commit()
    return {"vehicle_id": vehicle.vehicle_id, "display_name": vehicle.display_name}


@router.patch("/vehicles/{vehicle_id}")
async def update_vehicle(
    vehicle_id: str,
    request: VehicleUpdate,
    session: AsyncSession = Depends(get_session),
    _: User = Depends(require_roles("admin", "operator")),
) -> dict[str, Any]:
    vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == vehicle_id))
    if vehicle is None:
        raise HTTPException(status_code=404, detail="vehicle not found")
    values = request.model_dump(exclude_unset=True)
    if "metadata" in values:
        vehicle.metadata_json = values.pop("metadata")
    for key, value in values.items():
        setattr(vehicle, key, value)
    await session.commit()
    return {"vehicle_id": vehicle.vehicle_id, "updated": True}


@router.get("/vehicles/{vehicle_id}")
async def vehicle_detail(
    vehicle_id: str, session: AsyncSession = Depends(get_session)
) -> dict[str, Any]:
    vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == vehicle_id))
    if vehicle is None:
        raise HTTPException(status_code=404, detail="vehicle not found")
    latest = await latest_for_vehicle(session, vehicle)
    devices = list(
        (await session.scalars(select(Device).where(Device.vehicle_id == vehicle.id))).all()
    )
    alerts = list(
        (
            await session.scalars(
                select(Alert)
                .where(
                    Alert.vehicle_id == vehicle.id,
                    Alert.status.in_(["active", "acknowledged"]),
                )
                .order_by(Alert.triggered_at.desc())
            )
        ).all()
    )
    return {
        "vehicle": {
            "vehicle_id": vehicle.vehicle_id,
            "display_name": vehicle.display_name,
            "description": vehicle.description,
            "metadata": vehicle.metadata_json,
            "active": vehicle.active,
        },
        "devices": [
            {
                "device_id": item.device_id,
                "status": item.status,
                "status_reason": item.status_reason,
                "last_seen_at": iso(item.last_seen_at),
                "firmware_version": item.firmware_version,
                "simulated": item.simulated,
            }
            for item in devices
        ],
        "latest_telemetry": telemetry_data(latest),
        "active_alerts": [alert_data(item, vehicle.vehicle_id) for item in alerts],
    }


@router.get("/vehicles/{vehicle_id}/telemetry")
async def vehicle_telemetry(
    vehicle_id: str,
    hours: int = Query(default=24, ge=1, le=24 * 90),
    limit: int = Query(default=500, ge=1, le=5000),
    session: AsyncSession = Depends(get_session),
) -> list[dict[str, Any]]:
    vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == vehicle_id))
    if vehicle is None:
        raise HTTPException(status_code=404, detail="vehicle not found")
    cutoff = datetime.now(UTC) - timedelta(hours=hours)
    items = list(
        (
            await session.scalars(
                select(Telemetry)
                .where(Telemetry.vehicle_id == vehicle.id, Telemetry.received_at >= cutoff)
                .order_by(Telemetry.received_at.desc())
                .limit(limit)
            )
        ).all()
    )
    return [telemetry_data(item) for item in reversed(items)]


@router.get("/vehicles/{vehicle_id}/route")
async def vehicle_route(
    vehicle_id: str,
    hours: int = Query(default=6, ge=1, le=24 * 30),
    limit: int = Query(default=1000, ge=1, le=5000),
    session: AsyncSession = Depends(get_session),
) -> dict[str, Any]:
    vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == vehicle_id))
    if vehicle is None:
        raise HTTPException(status_code=404, detail="vehicle not found")
    cutoff = datetime.now(UTC) - timedelta(hours=hours)
    rows = list(
        (
            await session.scalars(
                select(Telemetry)
                .where(
                    Telemetry.vehicle_id == vehicle.id,
                    Telemetry.received_at >= cutoff,
                    Telemetry.gps_valid,
                )
                .order_by(Telemetry.received_at.asc())
                .limit(limit)
            )
        ).all()
    )
    return {
        "vehicle_id": vehicle.vehicle_id,
        "points": [
            {
                "latitude": item.latitude,
                "longitude": item.longitude,
                "speed_kmh": item.speed_kmh,
                "recorded_at": iso(item.measured_at or item.received_at),
                "replayed": item.replayed,
            }
            for item in rows
            if item.latitude is not None and item.longitude is not None
        ],
    }


@router.get("/vehicles/{vehicle_id}/acoustic")
async def vehicle_acoustic(
    vehicle_id: str,
    hours: int = Query(default=24, ge=1, le=24 * 90),
    limit: int = Query(default=1000, ge=1, le=5000),
    session: AsyncSession = Depends(get_session),
) -> list[dict[str, Any]]:
    vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == vehicle_id))
    if vehicle is None:
        raise HTTPException(status_code=404, detail="vehicle not found")
    cutoff = datetime.now(UTC) - timedelta(hours=hours)
    items = list(
        (
            await session.scalars(
                select(AcousticMeasurement)
                .where(
                    AcousticMeasurement.vehicle_id == vehicle.id,
                    AcousticMeasurement.received_at >= cutoff,
                )
                .order_by(AcousticMeasurement.received_at.desc())
                .limit(limit)
            )
        ).all()
    )
    return [
        {
            "sample_id": item.sample_id,
            "measured_at": iso(item.measured_at),
            "received_at": iso(item.received_at),
            "mic_valid": item.microphone_valid,
            "analysis_valid": item.analysis_valid,
            "relative_level_dbfs": item.relative_level_dbfs,
            "peak_dbfs": item.peak_dbfs,
            "clipping": item.clipping,
            "category": item.category,
            "confidence": item.confidence,
            "classifier_version": item.classifier_version,
            "features": item.features,
            "simulated": item.simulated,
        }
        for item in reversed(items)
    ]


@router.get("/devices")
async def list_devices(session: AsyncSession = Depends(get_session)) -> list[dict[str, Any]]:
    devices = list((await session.scalars(select(Device).order_by(Device.device_id))).all())
    vehicles = {item.id: item.vehicle_id for item in (await session.scalars(select(Vehicle))).all()}
    return [
        {
            "device_id": item.device_id,
            "vehicle_id": vehicles.get(item.vehicle_id),
            "display_name": item.display_name,
            "firmware_version": item.firmware_version,
            "status": item.status,
            "status_reason": item.status_reason,
            "last_seen_at": iso(item.last_seen_at),
            "simulated": item.simulated,
        }
        for item in devices
    ]


@router.post("/devices", status_code=201)
async def create_device(
    request: DeviceCreate,
    session: AsyncSession = Depends(get_session),
    _: User = Depends(require_roles("admin")),
) -> dict[str, Any]:
    vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == request.vehicle_id))
    if vehicle is None:
        raise HTTPException(status_code=404, detail="vehicle not found")
    if await session.scalar(select(Device.id).where(Device.device_id == request.device_id)):
        raise HTTPException(status_code=409, detail="device_id already exists")
    device = Device(
        device_id=request.device_id,
        vehicle_id=vehicle.id,
        display_name=request.display_name,
        status="unknown",
        simulated=request.simulated,
    )
    session.add(device)
    await session.commit()
    return {"device_id": device.device_id, "vehicle_id": vehicle.vehicle_id}


@router.get("/alerts")
async def list_alerts(
    status: str | None = Query(default=None, pattern="^(active|acknowledged|resolved)$"),
    limit: int = Query(default=100, ge=1, le=1000),
    session: AsyncSession = Depends(get_session),
) -> list[dict[str, Any]]:
    statement = select(Alert).order_by(Alert.triggered_at.desc()).limit(limit)
    if status:
        statement = statement.where(Alert.status == status)
    items = list((await session.scalars(statement)).all())
    vehicle_ids = {
        item.id: item.vehicle_id for item in (await session.scalars(select(Vehicle))).all()
    }
    return [alert_data(item, vehicle_ids.get(item.vehicle_id)) for item in items]


@router.patch("/alerts/{alert_id}")
async def update_alert(
    alert_id: str,
    request: AlertAction,
    session: AsyncSession = Depends(get_session),
    user: User = Depends(require_roles("admin", "operator")),
) -> dict[str, Any]:
    try:
        parsed_alert_id = UUID(alert_id)
    except ValueError:
        raise HTTPException(status_code=404, detail="alert not found") from None
    alert = await session.get(Alert, parsed_alert_id)
    if alert is None:
        raise HTTPException(status_code=404, detail="alert not found")
    now = datetime.now(UTC)
    if request.action == "acknowledge":
        if alert.status == "resolved":
            raise HTTPException(status_code=409, detail="resolved alert cannot be acknowledged")
        alert.status = "acknowledged"
        alert.acknowledged_at = now
        alert.acknowledged_by = user.id
    else:
        alert.status = "resolved"
        alert.resolved_at = now
    await session.commit()
    return {"id": str(alert.id), "status": alert.status}


@router.get("/trips")
async def list_trips(
    vehicle_id: str | None = None,
    limit: int = Query(default=100, ge=1, le=1000),
    session: AsyncSession = Depends(get_session),
) -> list[dict[str, Any]]:
    statement = select(Trip).order_by(Trip.started_at.desc()).limit(limit)
    vehicle = None
    if vehicle_id:
        vehicle = await session.scalar(select(Vehicle).where(Vehicle.vehicle_id == vehicle_id))
        if vehicle is None:
            raise HTTPException(status_code=404, detail="vehicle not found")
        statement = statement.where(Trip.vehicle_id == vehicle.id)
    items = list((await session.scalars(statement)).all())
    vehicle_ids = {
        item.id: item.vehicle_id for item in (await session.scalars(select(Vehicle))).all()
    }
    return [trip_data(item, vehicle_ids.get(item.vehicle_id)) for item in items]


@router.get("/trips/{trip_id}")
async def trip_detail(trip_id: str, session: AsyncSession = Depends(get_session)) -> dict[str, Any]:
    try:
        parsed_trip_id = UUID(trip_id)
    except ValueError:
        raise HTTPException(status_code=404, detail="trip not found") from None
    trip = await session.get(Trip, parsed_trip_id)
    if trip is None:
        raise HTTPException(status_code=404, detail="trip not found")
    vehicle = await session.get(Vehicle, trip.vehicle_id)
    points = list(
        (
            await session.scalars(
                select(TripPoint)
                .where(TripPoint.trip_id == trip.id)
                .order_by(TripPoint.recorded_at)
            )
        ).all()
    )
    return {
        **trip_data(trip, vehicle.vehicle_id if vehicle else None),
        "points": [
            {
                "latitude": item.latitude,
                "longitude": item.longitude,
                "speed_kmh": item.speed_kmh,
                "hdop": item.hdop,
                "recorded_at": iso(item.recorded_at),
            }
            for item in points
        ],
    }
