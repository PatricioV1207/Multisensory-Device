import { Download, FileJson, FileSpreadsheet, ShieldCheck } from "lucide-react";
import { useState } from "react";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { Panel } from "../components/ui/Panel";
import { useTelemetry, useVehicles } from "../hooks/queries";

function download(name: string, content: string, type: string) {
  const url = URL.createObjectURL(new Blob([content], { type }));
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = name;
  anchor.click();
  URL.revokeObjectURL(url);
}

export function ReportsPage() {
  const vehicles = useVehicles();
  const [selected, setSelected] = useState("");
  const selectedVehicle = selected || vehicles.data?.[0]?.vehicle_id || "";
  const telemetry = useTelemetry(selectedVehicle, 24);
  if (vehicles.isLoading) return <PageLoader label="Cargando reportes…" />;
  if (vehicles.isError)
    return <ErrorState retry={() => void vehicles.refetch()} />;
  if (!vehicles.data?.length)
    return <EmptyState title="No existen vehículos para exportar" />;
  const exportJson = () =>
    download(
      `${selectedVehicle}-telemetry.json`,
      JSON.stringify(telemetry.data ?? [], null, 2),
      "application/json",
    );
  const exportCsv = () => {
    const columns = [
      "measured_at",
      "received_at",
      "temperature_c",
      "humidity_percent",
      "pressure_hpa",
      "light_lux",
      "speed_kmh",
      "latitude",
      "longitude",
      "simulated",
    ] as const;
    const rows = (telemetry.data ?? []).map((item) =>
      columns.map((column) => JSON.stringify(item[column] ?? "")).join(","),
    );
    download(
      `${selectedVehicle}-telemetry.csv`,
      [columns.join(","), ...rows].join("\n"),
      "text/csv;charset=utf-8",
    );
  };
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Exportación</span>
          <h1>Reportes</h1>
          <p>
            Descarga las muestras que el backend conserva para el vehículo
            seleccionado.
          </p>
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
      <div className="report-grid">
        <Panel title="Telemetría CSV">
          <div className="report-card">
            <span>
              <FileSpreadsheet size={31} />
            </span>
            <p>
              Formato tabular para hojas de cálculo y análisis externo. Los
              campos ausentes quedan vacíos.
            </p>
            <button
              className="button button--primary"
              type="button"
              disabled={telemetry.isLoading || !telemetry.data?.length}
              onClick={exportCsv}
            >
              <Download size={16} /> Descargar CSV
            </button>
          </div>
        </Panel>
        <Panel title="Telemetría JSON">
          <div className="report-card">
            <span>
              <FileJson size={31} />
            </span>
            <p>
              Conserva estructura, banderas de validez y marcas
              `simulated`/`replayed`.
            </p>
            <button
              className="button button--secondary"
              type="button"
              disabled={telemetry.isLoading || !telemetry.data?.length}
              onClick={exportJson}
            >
              <Download size={16} /> Descargar JSON
            </button>
          </div>
        </Panel>
      </div>
      <Panel title="Alcance del reporte">
        <div className="information-note information-note--icon">
          <ShieldCheck size={21} />
          <p>
            La exportación usa las últimas 24 horas consultadas por REST. No se
            inventan valores para sensores inválidos y la hora de medición se
            conserva separada de la hora de recepción.
          </p>
        </div>
      </Panel>
    </div>
  );
}
