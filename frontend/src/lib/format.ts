import type { AlertState, DeviceState, Severity, Trip } from "../types/api";

export function numberOrDash(
  value: number | null | undefined,
  decimals = 1,
): string {
  return typeof value === "number" && Number.isFinite(value)
    ? value.toFixed(decimals)
    : "—";
}

export function valueWithUnit(
  value: number | null | undefined,
  unit: string,
  decimals = 1,
): string {
  const formatted = numberOrDash(value, decimals);
  return formatted === "—" ? formatted : `${formatted} ${unit}`;
}

export function compactDuration(seconds: number | null | undefined): string {
  if (seconds == null || !Number.isFinite(seconds)) return "En curso";
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  return hours > 0 ? `${hours} h ${minutes} min` : `${minutes} min`;
}

export function formatDateTime(value: string | null | undefined): string {
  if (!value) return "Sin datos";
  const parsed = new Date(value);
  if (Number.isNaN(parsed.getTime())) return "Sin datos";
  return new Intl.DateTimeFormat("es-EC", {
    dateStyle: "medium",
    timeStyle: "short",
  }).format(parsed);
}

export function timeAgo(
  value: string | null | undefined,
  now = Date.now(),
): string {
  if (!value) return "Sin actualizaciones";
  const parsed = new Date(value).getTime();
  if (!Number.isFinite(parsed)) return "Sin actualizaciones";
  const delta = Math.max(0, Math.floor((now - parsed) / 1000));
  if (delta < 10) return "Ahora";
  if (delta < 60) return `Hace ${delta} s`;
  if (delta < 3600) return `Hace ${Math.floor(delta / 60)} min`;
  if (delta < 86_400) return `Hace ${Math.floor(delta / 3600)} h`;
  return `Hace ${Math.floor(delta / 86_400)} d`;
}

export const stateLabels: Record<DeviceState, string> = {
  online: "En línea",
  stale: "Desactualizado",
  offline: "Sin conexión",
  reconnecting: "Reconectando",
  unknown: "Desconocido",
};

export const severityLabels: Record<Severity, string> = {
  low: "Baja",
  medium: "Media",
  high: "Alta",
  critical: "Crítica",
};

export const alertStateLabels: Record<AlertState, string> = {
  active: "Activa",
  acknowledged: "Reconocida",
  resolved: "Resuelta",
};

export const tripStateLabels: Record<Trip["status"], string> = {
  active: "En curso",
  completed: "Completado",
};

const alertTypeLabels: Record<string, string> = {
  acceleration_candidate: "Aceleración o frenado brusco",
  device_offline: "Dispositivo sin conexión",
  gps_invalid: "GPS sin fix válido",
  gps_quality_weak: "Calidad GPS débil",
  microphone_clipping: "Saturación acústica",
  microphone_failure: "Fallo de micrófono",
  prolonged_stop: "Parada prolongada",
  sensor_invalid: "Sensor no válido",
  storage_failure: "Fallo de microSD",
  telemetry_stale: "Telemetría desactualizada",
  temperature_out_of_range: "Temperatura fuera de rango",
  traffic_noise: "Ruido de tráfico sostenido",
};

export function alertTypeLabel(value: string): string {
  return alertTypeLabels[value] ?? humanize(value);
}

export function humanize(value: string | null | undefined): string {
  if (!value) return "Sin clasificar";
  return value
    .replaceAll("_", " ")
    .replace(/^./, (letter) => letter.toUpperCase());
}

export function vectorMagnitude(
  vector: [number, number, number] | null,
): number | null {
  if (!vector || vector.some((item) => !Number.isFinite(item))) return null;
  return Math.sqrt(vector.reduce((sum, item) => sum + item * item, 0));
}
