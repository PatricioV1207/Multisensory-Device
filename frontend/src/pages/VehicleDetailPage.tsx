import {
  Activity,
  AlertTriangle,
  ArrowLeft,
  AudioLines,
  Cloud,
  Cpu,
  Database,
  Droplets,
  Gauge,
  Lightbulb,
  MapPin,
  Navigation,
  Radio,
  Rotate3D,
  Satellite,
  Thermometer,
  Waves,
} from "lucide-react";
import { Link, useOutletContext, useParams } from "react-router-dom";
import { TelemetryChart } from "../components/charts/TelemetryChart";
import { RouteMap } from "../components/maps/VehicleMap";
import { DataValue } from "../components/ui/DataValue";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { LiveIndicator } from "../components/ui/LiveIndicator";
import { MetricCard } from "../components/ui/MetricCard";
import { Panel } from "../components/ui/Panel";
import {
  SeverityBadge,
  StatusBadge,
  ValidityBadge,
} from "../components/ui/StatusBadge";
import {
  useAcoustic,
  useRoute,
  useTelemetry,
  useVehicle,
} from "../hooks/queries";
import {
  formatDateTime,
  humanize,
  timeAgo,
  valueWithUnit,
  vectorMagnitude,
} from "../lib/format";
import type { ShellContext } from "../layout/AppShell";

function chartPoints(
  items: ReturnType<typeof useTelemetry>["data"],
  key: "temperature_c" | "speed_kmh" | "pressure_hpa",
) {
  return (items ?? []).map((item) => ({
    time: new Intl.DateTimeFormat("es-EC", {
      hour: "2-digit",
      minute: "2-digit",
    }).format(new Date(item.measured_at ?? item.received_at)),
    value: item[key],
  }));
}

export function VehicleDetailPage() {
  const { vehicleId = "" } = useParams();
  const vehicle = useVehicle(vehicleId);
  const telemetryHistory = useTelemetry(vehicleId, 24);
  const route = useRoute(vehicleId, 6);
  const acoustic = useAcoustic(vehicleId, 24);
  const { liveState } = useOutletContext<ShellContext>();
  if (vehicle.isLoading) return <PageLoader label="Cargando vehículo…" />;
  if (vehicle.isError || !vehicle.data) {
    return (
      <ErrorState
        title="No se encontró el vehículo"
        description="Comprueba el identificador o vuelve al listado."
        retry={() => void vehicle.refetch()}
      />
    );
  }
  const detail = vehicle.data;
  const latest = detail.latest_telemetry;
  const device = detail.devices[0];
  const acceleration = vectorMagnitude(latest?.acceleration_mps2 ?? null);
  const gyro = vectorMagnitude(latest?.gyroscope_rad_s ?? null);
  const magnetometer = vectorMagnitude(latest?.magnetometer_ut ?? null);
  const metadata = detail.vehicle.metadata;

  return (
    <div className="page vehicle-detail-page">
      <header className="vehicle-heading">
        <div>
          <Link className="back-link" to="/vehicles">
            <ArrowLeft size={15} /> Vehículos
          </Link>
          <div className="vehicle-heading__title">
            <h1>{detail.vehicle.display_name}</h1>
            {device && <StatusBadge state={device.status} />}
            {latest?.validity.gps && (
              <span className="badge badge--gps">
                <Navigation size={13} /> GPS válido
              </span>
            )}
          </div>
          <p>
            {detail.vehicle.vehicle_id} ·{" "}
            {device?.device_id ?? "Sin dispositivo asignado"}
          </p>
        </div>
        <div className="vehicle-heading__right">
          <LiveIndicator state={liveState} />
          <span>
            Última actualización:{" "}
            <strong>{timeAgo(latest?.received_at)}</strong>
          </span>
        </div>
      </header>

      <section className="metric-grid metric-grid--five">
        <MetricCard
          compact
          label="Velocidad"
          value={
            latest?.validity.gps
              ? valueWithUnit(latest.speed_kmh, "km/h", 0)
              : "—"
          }
          hint={latest?.validity.gps ? "Fuente GPS" : "Sin fix GPS"}
          icon={Gauge}
          tone="blue"
        />
        <MetricCard
          compact
          label="Temperatura"
          value={
            latest?.validity.dht
              ? valueWithUnit(latest.temperature_c, "°C")
              : "—"
          }
          hint={latest?.validity.dht ? "DHT11 válido" : "Sensor no válido"}
          icon={Thermometer}
          tone="cyan"
        />
        <MetricCard
          compact
          label="Humedad"
          value={
            latest?.validity.dht
              ? valueWithUnit(latest.humidity_percent, "%", 0)
              : "—"
          }
          hint={latest?.validity.dht ? "DHT11 válido" : "Sensor no válido"}
          icon={Droplets}
          tone="teal"
        />
        <MetricCard
          compact
          label="Nivel acústico"
          value={
            latest?.validity.acoustic
              ? valueWithUnit(latest.acoustic?.relative_level_dbfs, "dBFS")
              : "—"
          }
          hint={
            latest?.validity.acoustic
              ? humanize(latest.acoustic?.category)
              : "Análisis no válido"
          }
          icon={AudioLines}
          tone="purple"
        />
        <MetricCard
          compact
          label="Última muestra"
          value={latest ? timeAgo(latest.received_at) : "—"}
          hint={latest ? formatDateTime(latest.received_at) : "Sin telemetría"}
          icon={Radio}
          tone="green"
        />
      </section>

      <div className="vehicle-primary-grid">
        <Panel
          title="Ubicación y ruta reciente"
          eyebrow={
            latest?.validity.gps
              ? `${latest.satellites ?? "—"} satélites · HDOP ${latest.gps_hdop ?? "—"}`
              : "Sin fix GPS válido"
          }
          className="map-panel vehicle-route-panel"
        >
          {route.data?.points.length ? (
            <RouteMap
              points={route.data.points}
              title={detail.vehicle.display_name}
            />
          ) : (
            <div className="map-placeholder">
              <MapPin size={28} /> No existe una ruta GPS válida en la ventana
              seleccionada.
            </div>
          )}
        </Panel>
        <Panel title="Identidad y salud" className="vehicle-identity-panel">
          <div className="identity-hero">
            <span>
              <Cpu size={28} />
            </span>
            <div>
              <strong>
                {device?.display_name ?? device?.device_id ?? "Sin dispositivo"}
              </strong>
              <small>{detail.vehicle.description ?? "Sin descripción"}</small>
            </div>
          </div>
          <dl className="identity-list">
            <div>
              <dt>Identificador</dt>
              <dd>{device?.device_id ?? "—"}</dd>
            </div>
            <div>
              <dt>Firmware</dt>
              <dd>{device?.firmware_version ?? "—"}</dd>
            </div>
            <div>
              <dt>Último contacto</dt>
              <dd>{timeAgo(device?.last_seen_at)}</dd>
            </div>
            <div>
              <dt>Tipo</dt>
              <dd>{typeof metadata.type === "string" ? metadata.type : "—"}</dd>
            </div>
            <div>
              <dt>Placa (metadata)</dt>
              <dd>
                {typeof metadata.plate === "string" ? metadata.plate : "—"}
              </dd>
            </div>
            <div>
              <dt>Modo</dt>
              <dd>{latest?.simulated ? "Simulado" : "Producción"}</dd>
            </div>
          </dl>
          <div className="health-score">
            <Cloud size={19} />
            <div>
              <strong>{device ? humanize(device.status) : "Sin estado"}</strong>
              <span>Estado calculado por backend</span>
            </div>
          </div>
        </Panel>
      </div>

      <div className="vehicle-chart-grid">
        <Panel title="Velocidad (24 h)">
          <TelemetryChart
            data={chartPoints(telemetryHistory.data, "speed_kmh")}
            unit="km/h"
            label="Velocidad"
            color="#08a8a8"
          />
        </Panel>
        <Panel title="Temperatura (24 h)">
          <TelemetryChart
            data={chartPoints(telemetryHistory.data, "temperature_c")}
            unit="°C"
            label="Temperatura"
          />
        </Panel>
        <Panel title="Presión (24 h)">
          <TelemetryChart
            data={chartPoints(telemetryHistory.data, "pressure_hpa")}
            unit="hPa"
            label="Presión"
            color="#8b5cf6"
          />
        </Panel>
      </div>

      <div className="vehicle-data-grid">
        <Panel title="Telemetría actual" className="telemetry-panel">
          {!latest ? (
            <EmptyState title="Sin telemetría" />
          ) : (
            <>
              <div className="telemetry-values">
                <DataValue
                  icon={Thermometer}
                  label="Temperatura"
                  value={valueWithUnit(latest.temperature_c, "°C")}
                  valid={latest.validity.dht}
                />
                <DataValue
                  icon={Droplets}
                  label="Humedad"
                  value={valueWithUnit(latest.humidity_percent, "%")}
                  valid={latest.validity.dht}
                />
                <DataValue
                  icon={Activity}
                  label="Presión"
                  value={valueWithUnit(latest.pressure_hpa, "hPa")}
                  valid={latest.validity.barometer}
                />
                <DataValue
                  icon={Lightbulb}
                  label="Luz"
                  value={valueWithUnit(latest.light_lux, "lux", 0)}
                  valid={latest.validity.light}
                />
                <DataValue
                  icon={Waves}
                  label="Aceleración |a|"
                  value={valueWithUnit(acceleration, "m/s²", 2)}
                  valid={latest.validity.accel}
                />
                <DataValue
                  icon={Rotate3D}
                  label="Giroscopio |ω|"
                  value={valueWithUnit(gyro, "rad/s", 3)}
                  valid={latest.validity.gyro}
                />
                <DataValue
                  icon={Navigation}
                  label="Magnetómetro |B|"
                  value={valueWithUnit(magnetometer, "µT", 1)}
                  valid={latest.validity.mag}
                />
                <DataValue
                  icon={Satellite}
                  label="GPS"
                  value={
                    latest.validity.gps
                      ? `${latest.latitude?.toFixed(5)}, ${latest.longitude?.toFixed(5)}`
                      : "—"
                  }
                  valid={latest.validity.gps}
                />
                <DataValue
                  icon={Database}
                  label="microSD"
                  value="Respaldo disponible"
                  valid={latest.validity.storage}
                />
                <DataValue
                  icon={AudioLines}
                  label="Acústica"
                  value={
                    latest.acoustic
                      ? `${humanize(latest.acoustic.category)} · ${valueWithUnit(latest.acoustic.confidence == null ? null : latest.acoustic.confidence * 100, "%", 0)}`
                      : "—"
                  }
                  valid={latest.validity.acoustic}
                />
              </div>
              <div className="sensor-health-row">
                {Object.entries(latest.validity).map(([name, valid]) => (
                  <ValidityBadge
                    key={name}
                    label={humanize(name)}
                    valid={valid}
                  />
                ))}
              </div>
              <details className="technical-details">
                <summary>Ver telemetría técnica</summary>
                <pre>{JSON.stringify(latest, null, 2)}</pre>
              </details>
            </>
          )}
        </Panel>
        <Panel title="Eventos y acústica recientes">
          <div className="alert-list">
            {detail.active_alerts.map((alert) => (
              <Link to="/alerts" className="alert-row" key={alert.id}>
                <span
                  className={`alert-row__icon alert-row__icon--${alert.severity}`}
                >
                  <AlertTriangle size={18} />
                </span>
                <div>
                  <strong>{alert.title}</strong>
                  <span>
                    {timeAgo(alert.triggered_at)} · {alert.occurrence_count}{" "}
                    observación(es)
                  </span>
                </div>
                <SeverityBadge severity={alert.severity} />
              </Link>
            ))}
            {!detail.active_alerts.length && (
              <p className="panel-empty">No hay alertas activas.</p>
            )}
          </div>
          <div className="acoustic-summary">
            <header>
              <strong>Últimos análisis acústicos</strong>
              <span>Valores relativos, no dB SPL</span>
            </header>
            {(acoustic.data ?? [])
              .slice(-5)
              .reverse()
              .map((item) => (
                <div key={item.sample_id}>
                  <span>{humanize(item.category)}</span>
                  <strong>
                    {valueWithUnit(item.relative_level_dbfs, "dBFS")}
                  </strong>
                  <small>
                    {item.confidence == null
                      ? "—"
                      : `${Math.round(item.confidence * 100)}%`}
                  </small>
                </div>
              ))}
            {!acoustic.data?.length && (
              <p className="panel-empty">
                Sin análisis acústicos en la ventana.
              </p>
            )}
          </div>
        </Panel>
      </div>
    </div>
  );
}
