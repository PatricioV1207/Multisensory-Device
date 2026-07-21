import { MapPin, Navigation, Satellite } from "lucide-react";
import { Link } from "react-router-dom";
import { FleetMap } from "../components/maps/VehicleMap";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { Panel } from "../components/ui/Panel";
import { StatusBadge } from "../components/ui/StatusBadge";
import { useDashboard } from "../hooks/queries";
import { timeAgo, valueWithUnit } from "../lib/format";

export function LiveMapPage() {
  const dashboard = useDashboard();
  if (dashboard.isLoading) return <PageLoader label="Cargando posiciones…" />;
  if (dashboard.isError || !dashboard.data)
    return <ErrorState retry={() => void dashboard.refetch()} />;
  return (
    <div className="page map-page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">GPS</span>
          <h1>Mapa en vivo</h1>
          <p>Última posición válida recibida de cada vehículo.</p>
        </div>
        <span className="page-count">
          <Satellite size={15} /> {dashboard.data.positions.length} con fix
        </span>
      </header>
      <div className="full-map-layout">
        <Panel className="full-map-panel">
          {dashboard.data.positions.length ? (
            <FleetMap positions={dashboard.data.positions} />
          ) : (
            <EmptyState title="No hay posiciones GPS válidas" />
          )}
        </Panel>
        <Panel title="Vehículos visibles">
          <div className="map-vehicle-list">
            {dashboard.data.positions.map((position) => (
              <Link
                to={`/vehicles/${position.vehicle_id}`}
                key={position.vehicle_id}
              >
                <span className="map-vehicle-list__icon">
                  <Navigation size={17} />
                </span>
                <div>
                  <strong>{position.display_name}</strong>
                  <span>
                    <MapPin size={12} /> {position.latitude.toFixed(5)},{" "}
                    {position.longitude.toFixed(5)}
                  </span>
                  <small>
                    {valueWithUnit(position.speed_kmh, "km/h", 0)} ·{" "}
                    {timeAgo(position.received_at)}
                  </small>
                </div>
                <StatusBadge state={position.status} compact />
              </Link>
            ))}
          </div>
        </Panel>
      </div>
    </div>
  );
}
