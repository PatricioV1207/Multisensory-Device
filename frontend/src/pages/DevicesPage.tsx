import { Activity, Cpu, Radio, WifiOff } from "lucide-react";
import { Link } from "react-router-dom";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { StatusBadge } from "../components/ui/StatusBadge";
import { useDevices } from "../hooks/queries";
import { timeAgo } from "../lib/format";

export function DevicesPage() {
  const devices = useDevices();
  if (devices.isLoading) return <PageLoader label="Cargando dispositivos…" />;
  if (devices.isError)
    return <ErrorState retry={() => void devices.refetch()} />;
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Hardware</span>
          <h1>Dispositivos</h1>
          <p>Inventario, firmware y conectividad calculada.</p>
        </div>
        <span className="page-count">
          <Cpu size={15} /> {devices.data?.length ?? 0}
        </span>
      </header>
      {!devices.data?.length ? (
        <EmptyState title="No hay dispositivos registrados" />
      ) : (
        <div className="device-grid">
          {devices.data.map((device) => (
            <article className="device-card" key={device.device_id}>
              <header>
                <span
                  className={`device-card__icon device-card__icon--${device.status}`}
                >
                  {device.status === "offline" ? (
                    <WifiOff size={22} />
                  ) : (
                    <Activity size={22} />
                  )}
                </span>
                <StatusBadge state={device.status} />
              </header>
              <h2>{device.display_name ?? device.device_id}</h2>
              <p>{device.device_id}</p>
              <dl>
                <div>
                  <dt>Vehículo</dt>
                  <dd>
                    {device.vehicle_id ? (
                      <Link
                        className="text-link"
                        to={`/vehicles/${device.vehicle_id}`}
                      >
                        {device.vehicle_id}
                      </Link>
                    ) : (
                      "—"
                    )}
                  </dd>
                </div>
                <div>
                  <dt>Firmware</dt>
                  <dd>{device.firmware_version ?? "—"}</dd>
                </div>
                <div>
                  <dt>Último contacto</dt>
                  <dd>{timeAgo(device.last_seen_at)}</dd>
                </div>
                <div>
                  <dt>Origen</dt>
                  <dd>{device.simulated ? "Simulado" : "Producción"}</dd>
                </div>
              </dl>
              <footer>
                <Radio size={14} />{" "}
                {device.status_reason ?? "Sin razón reportada"}
              </footer>
            </article>
          ))}
        </div>
      )}
    </div>
  );
}
