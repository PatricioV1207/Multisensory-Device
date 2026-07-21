import {
  demoAcousticHistory,
  demoAlerts,
  demoDashboard,
  demoRoutes,
  demoTelemetryHistory,
  demoTrips,
  demoVehicleDetail,
  demoVehicles,
} from "../demo/data";
import type {
  AcousticMeasurement,
  AlertRecord,
  DashboardData,
  DeviceSummary,
  RouteData,
  Telemetry,
  TokenPair,
  Trip,
  UserProfile,
  VehicleDetail,
  VehicleListItem,
} from "../types/api";
import {
  clearTokens,
  getAccessToken,
  getRefreshToken,
  saveTokens,
} from "./session";

export const isDemoMode = import.meta.env.VITE_DEMO_MODE === "true";
const API_BASE = (import.meta.env.VITE_API_BASE_URL ?? "").replace(/\/$/, "");
const demoDelay = () => new Promise((resolve) => setTimeout(resolve, 40));

export class ApiError extends Error {
  constructor(
    public status: number,
    message: string,
  ) {
    super(message);
  }
}

async function parseError(response: Response): Promise<string> {
  try {
    const body = (await response.json()) as { detail?: string };
    return body.detail ?? `Error HTTP ${response.status}`;
  } catch {
    return `Error HTTP ${response.status}`;
  }
}

async function refreshSession(): Promise<boolean> {
  const refreshToken = getRefreshToken();
  if (!refreshToken) return false;
  const response = await fetch(`${API_BASE}/api/v1/auth/refresh`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ refresh_token: refreshToken }),
  });
  if (!response.ok) return false;
  saveTokens((await response.json()) as TokenPair);
  return true;
}

async function request<T>(
  path: string,
  init: RequestInit = {},
  retry = true,
): Promise<T> {
  const token = getAccessToken();
  const response = await fetch(`${API_BASE}${path}`, {
    ...init,
    headers: {
      ...(init.body ? { "Content-Type": "application/json" } : {}),
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
      ...init.headers,
    },
  });
  if (response.status === 401 && retry && (await refreshSession())) {
    return request<T>(path, init, false);
  }
  if (!response.ok) {
    if (response.status === 401) {
      clearTokens();
      window.dispatchEvent(new Event("vehiclesense:auth-expired"));
    }
    throw new ApiError(response.status, await parseError(response));
  }
  return (await response.json()) as T;
}

let mutableDemoAlerts = structuredClone(demoAlerts);

export const api = {
  async login(email: string, password: string): Promise<TokenPair> {
    if (isDemoMode) {
      await demoDelay();
      return {
        access_token: "demo-access",
        refresh_token: "demo-refresh",
        token_type: "bearer",
      };
    }
    return request<TokenPair>("/api/v1/auth/login", {
      method: "POST",
      body: JSON.stringify({ email, password }),
    });
  },
  async me(): Promise<UserProfile> {
    if (isDemoMode) {
      return {
        id: "demo-user",
        email: "demo@vehiclesense.local",
        role: "admin",
        active: true,
      };
    }
    return request<UserProfile>("/api/v1/auth/me");
  },
  async dashboard(): Promise<DashboardData> {
    if (isDemoMode) {
      await demoDelay();
      return {
        ...structuredClone(demoDashboard),
        recent_alerts: structuredClone(mutableDemoAlerts),
      };
    }
    return request<DashboardData>("/api/v1/dashboard");
  },
  async vehicles(): Promise<VehicleListItem[]> {
    if (isDemoMode) return demoVehicles();
    return request<VehicleListItem[]>("/api/v1/vehicles");
  },
  async vehicle(vehicleId: string): Promise<VehicleDetail> {
    if (isDemoMode) {
      const detail = demoVehicleDetail(vehicleId);
      if (!detail) throw new ApiError(404, "Vehículo no encontrado");
      detail.active_alerts = mutableDemoAlerts.filter(
        (alert) =>
          alert.vehicle_id === vehicleId && alert.status !== "resolved",
      );
      return detail;
    }
    return request<VehicleDetail>(
      `/api/v1/vehicles/${encodeURIComponent(vehicleId)}`,
    );
  },
  async telemetry(vehicleId: string, hours = 24): Promise<Telemetry[]> {
    if (isDemoMode) return demoTelemetryHistory(vehicleId);
    return request<Telemetry[]>(
      `/api/v1/vehicles/${encodeURIComponent(vehicleId)}/telemetry?hours=${hours}`,
    );
  },
  async route(vehicleId: string, hours = 6): Promise<RouteData> {
    if (isDemoMode) {
      return structuredClone(
        demoRoutes[vehicleId] ?? { vehicle_id: vehicleId, points: [] },
      );
    }
    return request<RouteData>(
      `/api/v1/vehicles/${encodeURIComponent(vehicleId)}/route?hours=${hours}`,
    );
  },
  async acoustic(
    vehicleId: string,
    hours = 24,
  ): Promise<AcousticMeasurement[]> {
    if (isDemoMode) return demoAcousticHistory(vehicleId);
    return request<AcousticMeasurement[]>(
      `/api/v1/vehicles/${encodeURIComponent(vehicleId)}/acoustic?hours=${hours}`,
    );
  },
  async devices(): Promise<DeviceSummary[]> {
    if (isDemoMode) return demoVehicles().flatMap((vehicle) => vehicle.devices);
    return request<DeviceSummary[]>("/api/v1/devices");
  },
  async alerts(status?: string): Promise<AlertRecord[]> {
    if (isDemoMode) return structuredClone(mutableDemoAlerts);
    return request<AlertRecord[]>(
      `/api/v1/alerts${status ? `?status=${status}` : ""}`,
    );
  },
  async updateAlert(
    alertId: string,
    action: "acknowledge" | "resolve",
  ): Promise<void> {
    if (isDemoMode) {
      mutableDemoAlerts = mutableDemoAlerts.map((alert) =>
        alert.id === alertId
          ? {
              ...alert,
              status: action === "acknowledge" ? "acknowledged" : "resolved",
              acknowledged_at:
                action === "acknowledge"
                  ? new Date().toISOString()
                  : alert.acknowledged_at,
              resolved_at:
                action === "resolve" ? new Date().toISOString() : null,
            }
          : alert,
      );
      return;
    }
    await request(`/api/v1/alerts/${encodeURIComponent(alertId)}`, {
      method: "PATCH",
      body: JSON.stringify({ action }),
    });
  },
  async trips(vehicleId?: string): Promise<Trip[]> {
    if (isDemoMode) {
      return structuredClone(
        vehicleId
          ? demoTrips.filter((trip) => trip.vehicle_id === vehicleId)
          : demoTrips,
      );
    }
    return request<Trip[]>(
      `/api/v1/trips${vehicleId ? `?vehicle_id=${vehicleId}` : ""}`,
    );
  },
};

export function resetDemoState(): void {
  mutableDemoAlerts = structuredClone(demoAlerts);
}
