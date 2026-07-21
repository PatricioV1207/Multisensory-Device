export type DeviceState =
  "online" | "stale" | "offline" | "reconnecting" | "unknown";
export type MotionState = "in_motion" | "stopped" | DeviceState;
export type Severity = "low" | "medium" | "high" | "critical";
export type AlertState = "active" | "acknowledged" | "resolved";

export interface Validity {
  dht: boolean;
  gps: boolean;
  accel: boolean;
  gyro: boolean;
  mag: boolean;
  barometer: boolean;
  imu: boolean;
  light: boolean;
  storage: boolean;
  microphone: boolean;
  acoustic: boolean;
}

export interface Telemetry {
  id: number;
  sample_id: string | null;
  schema_version: number;
  legacy: boolean;
  measured_at: string | null;
  received_at: string;
  replayed: boolean;
  simulated: boolean;
  validity: Validity;
  temperature_c: number | null;
  humidity_percent: number | null;
  latitude: number | null;
  longitude: number | null;
  gps_altitude_m: number | null;
  speed_kmh: number | null;
  satellites: number | null;
  gps_hdop: number | null;
  pressure_hpa: number | null;
  baro_altitude_m: number | null;
  light_lux: number | null;
  acceleration_mps2: [number, number, number] | null;
  gyroscope_rad_s: [number, number, number] | null;
  magnetometer_ut: [number, number, number] | null;
  acoustic: {
    relative_level_dbfs: number | null;
    category: string | null;
    confidence: number | null;
  } | null;
}

export interface DashboardVehicle {
  vehicle_id: string;
  display_name: string;
  status: DeviceState;
  motion_state: MotionState;
  last_update: string | null;
  latest_telemetry: Telemetry | null;
  devices: string[];
}

export interface Position {
  vehicle_id: string;
  display_name: string;
  latitude: number;
  longitude: number;
  speed_kmh: number | null;
  status: DeviceState;
  received_at: string;
}

export interface AlertRecord {
  id: string;
  vehicle_id: string | null;
  device_id: string | null;
  trip_id: string | null;
  type: string;
  severity: Severity;
  status: AlertState;
  title: string;
  triggered_at: string;
  last_observed_at: string;
  acknowledged_at: string | null;
  resolved_at: string | null;
  occurrence_count: number;
  details: Record<string, unknown>;
  simulated: boolean;
}

export interface Trip {
  id: string;
  vehicle_id: string | null;
  status: "active" | "completed";
  started_at: string;
  ended_at: string | null;
  duration_seconds: number | null;
  distance_km: number;
  average_speed_kmh: number | null;
  maximum_speed_kmh: number | null;
  point_count: number;
  start: [number, number] | null;
  end: [number, number] | null;
  simulated: boolean;
}

export interface DashboardData {
  generated_at: string;
  totals: {
    vehicles: number;
    online: number;
    in_motion: number;
    stopped: number;
    active_alerts: number;
  };
  vehicles: DashboardVehicle[];
  positions: Position[];
  recent_alerts: AlertRecord[];
  recent_trips: Trip[];
}

export interface DeviceSummary {
  device_id: string;
  vehicle_id?: string | null;
  display_name?: string | null;
  firmware_version: string | null;
  status: DeviceState;
  status_reason?: string | null;
  last_seen_at: string | null;
  simulated: boolean;
}

export interface VehicleListItem {
  vehicle_id: string;
  display_name: string;
  description: string | null;
  active: boolean;
  metadata: Record<string, unknown>;
  alert_thresholds: Record<string, unknown>;
  devices: DeviceSummary[];
  latest_telemetry: Telemetry | null;
}

export interface VehicleDetail {
  vehicle: {
    vehicle_id: string;
    display_name: string;
    description: string | null;
    metadata: Record<string, unknown>;
    active: boolean;
  };
  devices: DeviceSummary[];
  latest_telemetry: Telemetry | null;
  active_alerts: AlertRecord[];
}

export interface RoutePoint {
  latitude: number;
  longitude: number;
  speed_kmh: number | null;
  recorded_at: string;
  replayed: boolean;
}

export interface RouteData {
  vehicle_id: string;
  points: RoutePoint[];
}

export interface AcousticMeasurement {
  sample_id: string;
  measured_at: string | null;
  received_at: string;
  mic_valid: boolean;
  analysis_valid: boolean;
  relative_level_dbfs: number | null;
  peak_dbfs: number | null;
  clipping: boolean | null;
  category: string | null;
  confidence: number | null;
  classifier_version: string | null;
  features: Record<string, unknown>;
  simulated: boolean;
}

export interface UserProfile {
  id: string;
  email: string;
  role: "admin" | "operator" | "viewer";
  active: boolean;
}

export interface TokenPair {
  access_token: string;
  refresh_token: string;
  token_type: string;
}

export interface LiveMessage {
  type: string;
  vehicle_id?: string;
  [key: string]: unknown;
}
