from __future__ import annotations

import math
from datetime import datetime

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.core.config import Settings
from app.models import Device, Telemetry, Trip, TripPoint
from app.services.time_utils import ensure_utc


def haversine_km(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    radius_km = 6371.0088
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    delta_phi = math.radians(lat2 - lat1)
    delta_lambda = math.radians(lon2 - lon1)
    value = (
        math.sin(delta_phi / 2.0) ** 2
        + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda / 2.0) ** 2
    )
    return 2.0 * radius_km * math.atan2(math.sqrt(value), math.sqrt(1.0 - value))


class TripService:
    def __init__(self, settings: Settings) -> None:
        self.settings = settings

    async def process(
        self,
        session: AsyncSession,
        device: Device,
        telemetry: Telemetry,
        event_time: datetime,
    ) -> Trip | None:
        event_time = ensure_utc(event_time)
        if telemetry.replayed or not telemetry.gps_valid:
            return None
        if (
            telemetry.latitude is None
            or telemetry.longitude is None
            or telemetry.speed_kmh is None
            or (telemetry.gps_hdop is not None and telemetry.gps_hdop > 5.0)
        ):
            return None

        trip = await session.scalar(
            select(Trip)
            .where(Trip.vehicle_id == device.vehicle_id, Trip.status == "active")
            .order_by(Trip.started_at.desc())
            .limit(1)
        )
        speed = max(0.0, telemetry.speed_kmh)
        if trip is None:
            if speed < self.settings.trip_start_speed_kmh:
                device.motion_candidate_since = None
                return None
            if device.motion_candidate_since is None:
                device.motion_candidate_since = event_time
                return None
            duration = (event_time - ensure_utc(device.motion_candidate_since)).total_seconds()
            if duration < self.settings.trip_start_duration_seconds:
                return None
            trip = Trip(
                vehicle_id=device.vehicle_id,
                device_id=device.id,
                status="active",
                started_at=ensure_utc(device.motion_candidate_since),
                start_latitude=telemetry.latitude,
                start_longitude=telemetry.longitude,
                last_latitude=telemetry.latitude,
                last_longitude=telemetry.longitude,
                last_point_at=event_time,
                maximum_speed_kmh=speed,
                speed_sum_kmh=speed,
                speed_samples=1,
                point_count=1,
                simulated=telemetry.simulated,
            )
            session.add(trip)
            await session.flush()
            session.add(self._point(trip, telemetry, event_time))
            device.motion_candidate_since = None
            device.stopped_candidate_since = None
            return trip

        if trip.last_point_at is not None and event_time <= ensure_utc(trip.last_point_at):
            return trip
        if trip.last_latitude is not None and trip.last_longitude is not None:
            distance = haversine_km(
                trip.last_latitude,
                trip.last_longitude,
                telemetry.latitude,
                telemetry.longitude,
            )
            elapsed_hours = max(
                (
                    event_time - ensure_utc(trip.last_point_at)
                    if trip.last_point_at
                    else event_time - event_time
                ).total_seconds()
                / 3600.0,
                1.0 / 3600.0,
            )
            if distance / elapsed_hours > self.settings.trip_max_jump_speed_kmh:
                return trip
            trip.distance_km += distance

        session.add(self._point(trip, telemetry, event_time))
        trip.last_latitude = telemetry.latitude
        trip.last_longitude = telemetry.longitude
        trip.last_point_at = event_time
        trip.point_count += 1
        trip.speed_sum_kmh += speed
        trip.speed_samples += 1
        trip.maximum_speed_kmh = max(trip.maximum_speed_kmh or 0.0, speed)
        trip.average_speed_kmh = trip.speed_sum_kmh / trip.speed_samples

        if speed <= self.settings.trip_stop_speed_kmh:
            if device.stopped_candidate_since is None:
                device.stopped_candidate_since = event_time
            stopped_for = (event_time - ensure_utc(device.stopped_candidate_since)).total_seconds()
            if stopped_for >= self.settings.trip_stop_duration_seconds:
                trip.status = "completed"
                trip.ended_at = event_time
                trip.end_latitude = telemetry.latitude
                trip.end_longitude = telemetry.longitude
        else:
            device.stopped_candidate_since = None
        return trip

    @staticmethod
    def _point(trip: Trip, telemetry: Telemetry, event_time: datetime) -> TripPoint:
        return TripPoint(
            trip_id=trip.id,
            telemetry_id=telemetry.id,
            recorded_at=event_time,
            latitude=telemetry.latitude,
            longitude=telemetry.longitude,
            speed_kmh=telemetry.speed_kmh,
            hdop=telemetry.gps_hdop,
        )
