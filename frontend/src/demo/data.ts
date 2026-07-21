import type {
  AcousticMeasurement,
  AlertRecord,
  DashboardData,
  DeviceSummary,
  RouteData,
  Telemetry,
  Trip,
  VehicleDetail,
  VehicleListItem,
} from "../types/api";

const now = Date.now();
const isoAgo = (seconds: number) =>
  new Date(now - seconds * 1000).toISOString();

const valid = {
  dht: true,
  gps: true,
  accel: true,
  gyro: true,
  mag: true,
  barometer: true,
  imu: true,
  light: true,
  storage: true,
  microphone: true,
  acoustic: true,
};

function telemetry(
  id: number,
  vehicle: string,
  age: number,
  latitude: number,
  longitude: number,
  speed: number,
  temperature: number,
  category: string,
): Telemetry {
  return {
    id,
    sample_id: `${vehicle}:demo:${id}`,
    schema_version: 3,
    legacy: false,
    measured_at: isoAgo(age + 1),
    received_at: isoAgo(age),
    replayed: false,
    simulated: true,
    validity: { ...valid },
    temperature_c: temperature,
    humidity_percent: 61 - id * 3,
    latitude,
    longitude,
    gps_altitude_m: 7.8 + id,
    speed_kmh: speed,
    satellites: 9 + id,
    gps_hdop: 0.8 + id * 0.1,
    pressure_hpa: 1006.8 - id * 0.4,
    baro_altitude_m: 10.5 + id,
    light_lux: 330 + id * 52,
    acceleration_mps2: [0.12 * id, -0.05, 9.79],
    gyroscope_rad_s: [0.01, -0.02 * id, 0.03],
    magnetometer_ut: [25.6, -12.3, 48.9],
    acoustic: {
      relative_level_dbfs: -32 + id * 2,
      category,
      confidence: 0.68 + id * 0.07,
    },
  };
}

const telemetryByVehicle: Record<string, Telemetry> = {
  "vehicle-01": telemetry(
    1,
    "device-01",
    4,
    -2.1703,
    -79.9184,
    38.4,
    24.7,
    "traffic",
  ),
  "vehicle-02": telemetry(
    2,
    "device-02",
    18,
    -2.1636,
    -79.9069,
    0,
    26.1,
    "speech",
  ),
  "vehicle-03": {
    ...telemetry(3, "device-03", 225, -2.1832, -79.8957, 0, 29.4, "unknown"),
    validity: { ...valid, gps: false, microphone: false, acoustic: false },
    latitude: null,
    longitude: null,
    speed_kmh: null,
    satellites: null,
    gps_hdop: null,
    acoustic: null,
  },
};

export const demoAlerts: AlertRecord[] = [
  {
    id: "alert-demo-01",
    vehicle_id: "vehicle-01",
    device_id: "device-01",
    trip_id: "trip-demo-01",
    type: "acceleration_candidate",
    severity: "high",
    status: "active",
    title: "Candidato de frenado brusco",
    triggered_at: isoAgo(130),
    last_observed_at: isoAgo(110),
    acknowledged_at: null,
    resolved_at: null,
    occurrence_count: 2,
    details: { magnitude_mps2: 15.2 },
    simulated: true,
  },
  {
    id: "alert-demo-02",
    vehicle_id: "vehicle-03",
    device_id: "device-03",
    trip_id: null,
    type: "telemetry_stale",
    severity: "medium",
    status: "active",
    title: "Telemetría desactualizada",
    triggered_at: isoAgo(225),
    last_observed_at: isoAgo(225),
    acknowledged_at: null,
    resolved_at: null,
    occurrence_count: 1,
    details: { reason: "telemetry_stale" },
    simulated: true,
  },
  {
    id: "alert-demo-03",
    vehicle_id: "vehicle-02",
    device_id: "device-02",
    trip_id: null,
    type: "gps_quality_weak",
    severity: "low",
    status: "acknowledged",
    title: "Calidad GPS débil",
    triggered_at: isoAgo(960),
    last_observed_at: isoAgo(900),
    acknowledged_at: isoAgo(600),
    resolved_at: null,
    occurrence_count: 4,
    details: { hdop: 2.9 },
    simulated: true,
  },
];

export const demoTrips: Trip[] = [
  {
    id: "trip-demo-01",
    vehicle_id: "vehicle-01",
    status: "active",
    started_at: isoAgo(2700),
    ended_at: null,
    duration_seconds: null,
    distance_km: 18.7,
    average_speed_kmh: 31.5,
    maximum_speed_kmh: 58.2,
    point_count: 89,
    start: [-2.1894, -79.8891],
    end: null,
    simulated: true,
  },
  {
    id: "trip-demo-02",
    vehicle_id: "vehicle-02",
    status: "completed",
    started_at: isoAgo(13_800),
    ended_at: isoAgo(10_200),
    duration_seconds: 3600,
    distance_km: 24.3,
    average_speed_kmh: 27.2,
    maximum_speed_kmh: 51.1,
    point_count: 122,
    start: [-2.153, -79.883],
    end: [-2.1636, -79.9069],
    simulated: true,
  },
];

const vehicles: VehicleListItem[] = [
  ["vehicle-01", "Vehículo 01", "online"],
  ["vehicle-02", "Vehículo 02", "online"],
  ["vehicle-03", "Vehículo 03", "stale"],
].map(([vehicleId, name, state], index) => ({
  vehicle_id: vehicleId,
  display_name: name,
  description: `Unidad de demostración ${index + 1}`,
  active: true,
  metadata: {
    type: index === 1 ? "Sedán" : "SUV",
    plate: `DEMO-00${index + 1}`,
  },
  alert_thresholds: {},
  devices: [
    {
      device_id: `device-0${index + 1}`,
      status: state,
      last_seen_at: telemetryByVehicle[vehicleId].received_at,
      firmware_version: "3.0.0-demo",
      simulated: true,
    } as DeviceSummary,
  ],
  latest_telemetry: telemetryByVehicle[vehicleId],
})) as VehicleListItem[];

export const demoDashboard: DashboardData = {
  generated_at: new Date(now).toISOString(),
  totals: {
    vehicles: 3,
    online: 2,
    in_motion: 1,
    stopped: 1,
    active_alerts: 3,
  },
  vehicles: vehicles.map((vehicle, index) => ({
    vehicle_id: vehicle.vehicle_id,
    display_name: vehicle.display_name,
    status: index < 2 ? "online" : "stale",
    motion_state: index === 0 ? "in_motion" : index === 1 ? "stopped" : "stale",
    last_update: vehicle.latest_telemetry?.received_at ?? null,
    latest_telemetry: vehicle.latest_telemetry,
    devices: vehicle.devices.map((device) => device.device_id),
  })),
  positions: vehicles
    .filter((vehicle) => vehicle.latest_telemetry?.validity.gps)
    .map((vehicle) => ({
      vehicle_id: vehicle.vehicle_id,
      display_name: vehicle.display_name,
      latitude: vehicle.latest_telemetry!.latitude!,
      longitude: vehicle.latest_telemetry!.longitude!,
      speed_kmh: vehicle.latest_telemetry!.speed_kmh,
      status: "online",
      received_at: vehicle.latest_telemetry!.received_at,
    })),
  recent_alerts: demoAlerts,
  recent_trips: demoTrips,
};

export const demoRoutes: Record<string, RouteData> = Object.fromEntries(
  vehicles.map((vehicle, vehicleIndex) => {
    const current = telemetryByVehicle[vehicle.vehicle_id];
    if (
      !current.validity.gps ||
      current.latitude == null ||
      current.longitude == null
    ) {
      return [
        vehicle.vehicle_id,
        { vehicle_id: vehicle.vehicle_id, points: [] },
      ];
    }
    const points = Array.from({ length: 20 }, (_, index) => ({
      latitude: current.latitude! - (19 - index) * 0.0011,
      longitude:
        current.longitude! +
        Math.sin(index / 3) * 0.0017 -
        (19 - index) * 0.0004,
      speed_kmh: Math.max(0, (current.speed_kmh ?? 0) + Math.sin(index) * 8),
      recorded_at: isoAgo((19 - index) * 120 + vehicleIndex * 20),
      replayed: false,
    }));
    return [vehicle.vehicle_id, { vehicle_id: vehicle.vehicle_id, points }];
  }),
);

export function demoVehicles(): VehicleListItem[] {
  return structuredClone(vehicles);
}

export function demoVehicleDetail(vehicleId: string): VehicleDetail | null {
  const vehicle = vehicles.find((item) => item.vehicle_id === vehicleId);
  if (!vehicle) return null;
  return {
    vehicle: {
      vehicle_id: vehicle.vehicle_id,
      display_name: vehicle.display_name,
      description: vehicle.description,
      metadata: vehicle.metadata,
      active: vehicle.active,
    },
    devices: structuredClone(vehicle.devices),
    latest_telemetry: structuredClone(vehicle.latest_telemetry),
    active_alerts: structuredClone(
      demoAlerts.filter(
        (alert) =>
          alert.vehicle_id === vehicleId && alert.status !== "resolved",
      ),
    ),
  };
}

export function demoTelemetryHistory(vehicleId: string): Telemetry[] {
  const latest = telemetryByVehicle[vehicleId];
  if (!latest) return [];
  return Array.from({ length: 30 }, (_, index) => ({
    ...structuredClone(latest),
    id: latest.id * 100 + index,
    sample_id: `${latest.sample_id}:history:${index}`,
    measured_at: isoAgo((29 - index) * 300),
    received_at: isoAgo((29 - index) * 300 - 2),
    temperature_c:
      latest.temperature_c == null
        ? null
        : latest.temperature_c + Math.sin(index / 4) * 2.1,
    humidity_percent:
      latest.humidity_percent == null
        ? null
        : latest.humidity_percent + Math.cos(index / 5) * 4,
    speed_kmh:
      latest.speed_kmh == null
        ? null
        : Math.max(0, latest.speed_kmh + Math.sin(index / 2) * 14),
    pressure_hpa:
      latest.pressure_hpa == null
        ? null
        : latest.pressure_hpa + Math.sin(index / 7) * 1.2,
  }));
}

export function demoAcousticHistory(vehicleId: string): AcousticMeasurement[] {
  if (!telemetryByVehicle[vehicleId]) return [];
  const categories = ["quiet", "engine", "traffic", "speech", "unknown"];
  return Array.from({ length: 20 }, (_, index) => ({
    sample_id: `${vehicleId}:acoustic:${index}`,
    measured_at: isoAgo((19 - index) * 120),
    received_at: isoAgo((19 - index) * 120 - 1),
    mic_valid: true,
    analysis_valid: true,
    relative_level_dbfs: -44 + Math.sin(index / 2) * 12,
    peak_dbfs: -18 + Math.sin(index / 2) * 5,
    clipping: false,
    category: categories[index % categories.length],
    confidence: 0.56 + (index % 4) * 0.09,
    classifier_version: "heuristic-v1-demo",
    features: {},
    simulated: true,
  }));
}
