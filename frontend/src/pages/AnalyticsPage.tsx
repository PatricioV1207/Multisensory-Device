import { Activity, AudioLines, Gauge, Thermometer } from "lucide-react";
import { useState } from "react";
import { TelemetryChart } from "../components/charts/TelemetryChart";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { MetricCard } from "../components/ui/MetricCard";
import { Panel } from "../components/ui/Panel";
import { useAcoustic, useTelemetry, useVehicles } from "../hooks/queries";
import { humanize, numberOrDash } from "../lib/format";

export function AnalyticsPage() {
  const vehicles = useVehicles();
  const [selected, setSelected] = useState("");
  const selectedVehicle = selected || vehicles.data?.[0]?.vehicle_id || "";
  const telemetry = useTelemetry(selectedVehicle, 24);
  const acoustic = useAcoustic(selectedVehicle, 24);
  if (vehicles.isLoading) return <PageLoader label="Preparando analítica…" />;
  if (vehicles.isError)
    return <ErrorState retry={() => void vehicles.refetch()} />;
  if (!vehicles.data?.length)
    return <EmptyState title="No hay vehículos para analizar" />;
  const chart = (
    key: "temperature_c" | "humidity_percent" | "speed_kmh" | "pressure_hpa",
  ) =>
    (telemetry.data ?? []).map((item) => ({
      time: new Date(item.measured_at ?? item.received_at).toLocaleTimeString(
        "es-EC",
        { hour: "2-digit", minute: "2-digit" },
      ),
      value: item[key],
    }));
  const latest = telemetry.data?.at(-1);
  const acousticValid = (acoustic.data ?? []).filter(
    (item) => item.analysis_valid && item.relative_level_dbfs != null,
  );
  const acousticAverage = acousticValid.length
    ? acousticValid.reduce((sum, item) => sum + item.relative_level_dbfs!, 0) /
      acousticValid.length
    : null;
  const dominant = Object.entries(
    (acoustic.data ?? []).reduce<Record<string, number>>((counts, item) => {
      const category = item.category ?? "unknown";
      counts[category] = (counts[category] ?? 0) + 1;
      return counts;
    }, {}),
  ).sort((a, b) => b[1] - a[1])[0]?.[0];
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Histórico</span>
          <h1>Analítica</h1>
          <p>Tendencias de las últimas 24 horas por vehículo.</p>
        </div>
        <label className="select-control">
          Vehículo
          <select
            value={selectedVehicle}
            onChange={(event) => setSelected(event.target.value)}
          >
            {vehicles.data.map((vehicle) => (
              <option key={vehicle.vehicle_id} value={vehicle.vehicle_id}>
                {vehicle.display_name}
              </option>
            ))}
          </select>
        </label>
      </header>
      <section className="metric-grid metric-grid--four">
        <MetricCard
          compact
          label="Temperatura actual"
          value={
            latest?.temperature_c == null
              ? "—"
              : `${numberOrDash(latest.temperature_c)} °C`
          }
          icon={Thermometer}
          tone="blue"
        />
        <MetricCard
          compact
          label="Velocidad actual"
          value={
            latest?.speed_kmh == null
              ? "—"
              : `${numberOrDash(latest.speed_kmh, 0)} km/h`
          }
          icon={Gauge}
          tone="teal"
        />
        <MetricCard
          compact
          label="Nivel acústico medio"
          value={
            acousticAverage == null
              ? "—"
              : `${numberOrDash(acousticAverage)} dBFS`
          }
          icon={AudioLines}
          tone="purple"
        />
        <MetricCard
          compact
          label="Categoría frecuente"
          value={humanize(dominant)}
          icon={Activity}
          tone="green"
        />
      </section>
      {telemetry.isLoading ? (
        <PageLoader label="Cargando series…" />
      ) : telemetry.isError ? (
        <ErrorState retry={() => void telemetry.refetch()} />
      ) : (
        <div className="analytics-chart-grid">
          <Panel title="Temperatura">
            <TelemetryChart
              data={chart("temperature_c")}
              unit="°C"
              label="Temperatura"
            />
          </Panel>
          <Panel title="Humedad">
            <TelemetryChart
              data={chart("humidity_percent")}
              unit="%"
              label="Humedad"
              color="#08a8a8"
            />
          </Panel>
          <Panel title="Velocidad GPS">
            <TelemetryChart
              data={chart("speed_kmh")}
              unit="km/h"
              label="Velocidad"
              color="#16b876"
            />
          </Panel>
          <Panel title="Presión local">
            <TelemetryChart
              data={chart("pressure_hpa")}
              unit="hPa"
              label="Presión"
              color="#8b5cf6"
            />
          </Panel>
        </div>
      )}
      <Panel title="Nota de interpretación">
        <p className="information-note">
          Las mediciones acústicas se expresan en dBFS relativos al sistema
          digital. No son dB SPL calibrados y la categoría heurística no implica
          una clasificación validada en campo.
        </p>
      </Panel>
    </div>
  );
}
