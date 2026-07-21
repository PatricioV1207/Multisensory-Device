import {
  AlertTriangle,
  ArrowRight,
  CarFront,
  CirclePause,
  Gauge,
  Radio,
  Route,
  ShieldAlert,
  Thermometer,
  Truck,
  Wifi,
} from "lucide-react";
import { Link, useOutletContext } from "react-router-dom";
import { ConnectivityDonut } from "../components/charts/ConnectivityDonut";
import { TelemetryChart } from "../components/charts/TelemetryChart";
import { FleetMap } from "../components/maps/VehicleMap";
import { ErrorState, PageLoader } from "../components/ui/AsyncState";
import { LiveIndicator } from "../components/ui/LiveIndicator";
import { MetricCard } from "../components/ui/MetricCard";
import { Panel } from "../components/ui/Panel";
import { SeverityBadge, StatusBadge } from "../components/ui/StatusBadge";
import { useDashboard } from "../hooks/queries";
import {
  compactDuration,
  timeAgo,
  tripStateLabels,
  valueWithUnit,
} from "../lib/format";
import type { ShellContext } from "../layout/AppShell";
import type { DashboardVehicle } from "../types/api";

function VehicleCard({ vehicle }: { vehicle: DashboardVehicle }) {
  const telemetry = vehicle.latest_telemetry;
  return (
    <Link className="vehicle-summary" to={`/vehicles/${vehicle.vehicle_id}`}>
      <div className="vehicle-summary__header">
        <span className="vehicle-summary__icon">
          <CarFront size={21} />
        </span>
        <div>
          <strong>{vehicle.display_name}</strong>
          <span>{vehicle.vehicle_id}</span>
        </div>
        <StatusBadge state={vehicle.status} compact />
      </div>
      <div className="vehicle-summary__metrics">
        <span>
          <Thermometer size={14} />{" "}
          {valueWithUnit(telemetry?.temperature_c, "°C")}
        </span>
        <span>
          <Gauge size={14} />{" "}
          {telemetry?.validity.gps
            ? valueWithUnit(telemetry.speed_kmh, "km/h", 0)
            : "Sin GPS"}
        </span>
        <span>
          <Wifi size={14} /> {timeAgo(vehicle.last_update)}
        </span>
      </div>
    </Link>
  );
}

export function DashboardPage() {
  const dashboard = useDashboard();
  const { liveState } = useOutletContext<ShellContext>();
  if (dashboard.isLoading)
    return <PageLoader label="Cargando estado de la flota…" />;
  if (dashboard.isError || !dashboard.data) {
    return <ErrorState retry={() => void dashboard.refetch()} />;
  }
  const data = dashboard.data;
  const temperatureChart = data.vehicles.map((vehicle) => ({
    time: vehicle.display_name.replace("Vehículo ", "V"),
    value: vehicle.latest_telemetry?.validity.dht
      ? (vehicle.latest_telemetry.temperature_c ?? null)
      : null,
  }));
  const speedChart = data.vehicles.map((vehicle) => ({
    time: vehicle.display_name.replace("Vehículo ", "V"),
    value: vehicle.latest_telemetry?.validity.gps
      ? (vehicle.latest_telemetry.speed_kmh ?? null)
      : null,
  }));

  return (
    <div className="page dashboard-page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Centro de operaciones</span>
          <h1>Dashboard de flota</h1>
          <p>Estado actual de vehículos, dispositivos y eventos.</p>
        </div>
        <LiveIndicator state={liveState} />
      </header>

      <section
        className="metric-grid metric-grid--five"
        aria-label="Resumen de flota"
      >
        <MetricCard
          label="Vehículos"
          value={data.totals.vehicles}
          hint="Registrados y activos"
          icon={Truck}
          tone="blue"
        />
        <MetricCard
          label="En línea"
          value={data.totals.online}
          hint={
            data.totals.vehicles
              ? `${Math.round((data.totals.online / data.totals.vehicles) * 100)}% de la flota`
              : "Sin vehículos"
          }
          icon={Radio}
          tone="green"
        />
        <MetricCard
          label="En movimiento"
          value={data.totals.in_motion}
          hint="Según velocidad GPS"
          icon={Gauge}
          tone="teal"
        />
        <MetricCard
          label="Detenidos"
          value={data.totals.stopped}
          hint="Con telemetría vigente"
          icon={CirclePause}
          tone="orange"
        />
        <MetricCard
          label="Alertas activas"
          value={data.totals.active_alerts}
          hint="Requieren revisión"
          icon={ShieldAlert}
          tone="red"
        />
      </section>

      <div className="dashboard-main-grid">
        <Panel
          title="Mapa en vivo"
          eyebrow={`${data.positions.length} posiciones GPS válidas`}
          action={
            <Link className="text-link" to="/map">
              Abrir mapa <ArrowRight size={14} />
            </Link>
          }
          className="map-panel"
        >
          {data.positions.length ? (
            <FleetMap positions={data.positions} />
          ) : (
            <div className="map-placeholder">
              No hay posiciones GPS válidas.
            </div>
          )}
        </Panel>
        <Panel
          title="Alertas recientes"
          action={
            <Link className="text-link" to="/alerts">
              Ver todas
            </Link>
          }
        >
          <div className="alert-list">
            {data.recent_alerts.slice(0, 5).map((alert) => (
              <Link to="/alerts" className="alert-row" key={alert.id}>
                <span
                  className={`alert-row__icon alert-row__icon--${alert.severity}`}
                >
                  <AlertTriangle size={18} />
                </span>
                <div>
                  <strong>{alert.title}</strong>
                  <span>
                    {alert.vehicle_id ?? "Sin vehículo"} ·{" "}
                    {timeAgo(alert.triggered_at)}
                  </span>
                </div>
                <SeverityBadge severity={alert.severity} />
              </Link>
            ))}
            {!data.recent_alerts.length && (
              <p className="panel-empty">No hay alertas registradas.</p>
            )}
          </div>
        </Panel>
      </div>

      <Panel
        title="Resumen de vehículos"
        action={
          <Link className="text-link" to="/vehicles">
            Ver vehículos <ArrowRight size={14} />
          </Link>
        }
      >
        <div className="vehicle-summary-grid">
          {data.vehicles.slice(0, 4).map((vehicle) => (
            <VehicleCard key={vehicle.vehicle_id} vehicle={vehicle} />
          ))}
        </div>
      </Panel>

      <div className="dashboard-lower-grid">
        <Panel title="Analítica instantánea" className="analytics-panel">
          <div className="mini-chart-grid">
            <div className="mini-chart">
              <header>
                <strong>Temperatura por vehículo</strong>
                <span>°C</span>
              </header>
              <TelemetryChart
                data={temperatureChart}
                unit="°C"
                label="Temperatura"
              />
            </div>
            <div className="mini-chart">
              <header>
                <strong>Velocidad actual</strong>
                <span>km/h</span>
              </header>
              <TelemetryChart
                data={speedChart}
                unit="km/h"
                label="Velocidad"
                color="#08a8a8"
              />
            </div>
            <div className="connectivity-card">
              <ConnectivityDonut
                online={data.totals.online}
                total={data.totals.vehicles}
              />
              <div>
                <strong>Conectividad</strong>
                <span>{data.totals.online} en línea</span>
                <span>
                  {Math.max(0, data.totals.vehicles - data.totals.online)} no
                  disponibles
                </span>
              </div>
            </div>
          </div>
        </Panel>
        <Panel
          title="Viajes recientes"
          action={
            <Link className="text-link" to="/trips">
              Ver todos
            </Link>
          }
        >
          <div className="trip-list">
            {data.recent_trips.slice(0, 5).map((trip) => (
              <div className="trip-row" key={trip.id}>
                <span className="trip-row__icon">
                  <Route size={17} />
                </span>
                <div>
                  <strong>{trip.vehicle_id ?? "Sin vehículo"}</strong>
                  <span>
                    {valueWithUnit(trip.distance_km, "km")} ·{" "}
                    {compactDuration(trip.duration_seconds)}
                  </span>
                </div>
                <span className={`trip-state trip-state--${trip.status}`}>
                  {tripStateLabels[trip.status]}
                </span>
              </div>
            ))}
            {!data.recent_trips.length && (
              <p className="panel-empty">Todavía no hay viajes detectados.</p>
            )}
          </div>
        </Panel>
      </div>
    </div>
  );
}
