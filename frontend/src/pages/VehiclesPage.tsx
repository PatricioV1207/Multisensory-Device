import {
  ArrowRight,
  CarFront,
  Gauge,
  Search,
  Thermometer,
  Wifi,
} from "lucide-react";
import { Link, useSearchParams } from "react-router-dom";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { StatusBadge } from "../components/ui/StatusBadge";
import { useVehicles } from "../hooks/queries";
import { timeAgo, valueWithUnit } from "../lib/format";

export function VehiclesPage() {
  const vehicles = useVehicles();
  const [params, setParams] = useSearchParams();
  const search = params.get("search") ?? "";
  if (vehicles.isLoading) return <PageLoader label="Cargando vehículos…" />;
  if (vehicles.isError)
    return <ErrorState retry={() => void vehicles.refetch()} />;
  const filtered = (vehicles.data ?? []).filter((vehicle) => {
    const text =
      `${vehicle.display_name} ${vehicle.vehicle_id} ${vehicle.devices.map((item) => item.device_id).join(" ")}`.toLowerCase();
    return text.includes(search.toLowerCase());
  });
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Inventario</span>
          <h1>Vehículos</h1>
          <p>Identidad, telemetría reciente y dispositivos asignados.</p>
        </div>
        <span className="page-count">{filtered.length} resultado(s)</span>
      </header>
      <div className="page-toolbar">
        <label className="filter-search">
          <Search size={17} />
          <input
            value={search}
            onChange={(event) =>
              setParams(
                event.target.value ? { search: event.target.value } : {},
              )
            }
            placeholder="Buscar por vehículo o dispositivo"
          />
        </label>
      </div>
      {!filtered.length ? (
        <EmptyState
          title="No hay vehículos que coincidan"
          description="Cambia la búsqueda o registra un vehículo desde la API."
        />
      ) : (
        <div className="vehicle-catalog">
          {filtered.map((vehicle) => {
            const latest = vehicle.latest_telemetry;
            const device = vehicle.devices[0];
            return (
              <article className="vehicle-card" key={vehicle.vehicle_id}>
                <header>
                  <span className="vehicle-card__image">
                    <CarFront size={34} />
                  </span>
                  <div>
                    <h2>{vehicle.display_name}</h2>
                    <p>{vehicle.vehicle_id}</p>
                  </div>
                  {device && <StatusBadge state={device.status} />}
                </header>
                <p className="vehicle-card__description">
                  {vehicle.description ?? "Sin descripción"}
                </p>
                <div className="vehicle-card__data">
                  <span>
                    <Thermometer size={16} />
                    <small>Temperatura</small>
                    <strong>
                      {latest?.validity.dht
                        ? valueWithUnit(latest.temperature_c, "°C")
                        : "No válida"}
                    </strong>
                  </span>
                  <span>
                    <Gauge size={16} />
                    <small>Velocidad</small>
                    <strong>
                      {latest?.validity.gps
                        ? valueWithUnit(latest.speed_kmh, "km/h", 0)
                        : "Sin GPS"}
                    </strong>
                  </span>
                  <span>
                    <Wifi size={16} />
                    <small>Actualización</small>
                    <strong>{timeAgo(latest?.received_at)}</strong>
                  </span>
                </div>
                <footer>
                  <span>{vehicle.devices.length} dispositivo(s)</span>
                  <Link
                    className="text-link"
                    to={`/vehicles/${vehicle.vehicle_id}`}
                  >
                    Ver detalle <ArrowRight size={14} />
                  </Link>
                </footer>
              </article>
            );
          })}
        </div>
      )}
    </div>
  );
}
