import { CalendarDays, Clock3, Gauge, MapPin, Route } from "lucide-react";
import { useState } from "react";
import { Link } from "react-router-dom";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { useTrips } from "../hooks/queries";
import {
  compactDuration,
  formatDateTime,
  tripStateLabels,
  valueWithUnit,
} from "../lib/format";

export function TripsPage() {
  const [days, setDays] = useState(7);
  const trips = useTrips(undefined, days);
  if (trips.isLoading) return <PageLoader label="Cargando viajes…" />;
  if (trips.isError) return <ErrorState retry={() => void trips.refetch()} />;
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Movimiento GPS</span>
          <h1>Viajes</h1>
          <p>
            Trayectos guardados automáticamente a partir de posiciones GPS
            válidas; no representan encendido.
          </p>
        </div>
        <span className="page-count">
          <Route size={15} /> {trips.data?.length ?? 0}
        </span>
      </header>
      <div className="page-toolbar" aria-label="Periodo de viajes">
        <span className="toolbar-label">
          <CalendarDays size={15} /> Periodo
        </span>
        {[
          [1, "24 horas"],
          [7, "7 días"],
          [30, "30 días"],
        ].map(([value, label]) => (
          <button
            type="button"
            key={value}
            className={`filter-chip ${days === value ? "filter-chip--active" : ""}`}
            onClick={() => setDays(Number(value))}
          >
            {label}
          </button>
        ))}
      </div>
      {!trips.data?.length ? (
        <EmptyState
          title="Todavía no hay viajes"
          description="Se necesita movimiento GPS sostenido para iniciar un viaje."
        />
      ) : (
        <div className="trip-cards">
          {trips.data.map((trip) => (
            <article className="trip-card" key={trip.id}>
              <header>
                <span>
                  <Route size={21} />
                </span>
                <div>
                  <Link to={`/trips/${trip.id}`}>
                    {trip.vehicle_id ?? "Sin vehículo"}
                  </Link>
                  <small>{formatDateTime(trip.started_at)}</small>
                </div>
                <span className={`trip-state trip-state--${trip.status}`}>
                  {tripStateLabels[trip.status]}
                </span>
              </header>
              <div className="trip-card__stats">
                <span>
                  <MapPin size={16} />
                  <div>
                    <small>Distancia</small>
                    <strong>{valueWithUnit(trip.distance_km, "km")}</strong>
                  </div>
                </span>
                <span>
                  <Clock3 size={16} />
                  <div>
                    <small>Duración</small>
                    <strong>{compactDuration(trip.duration_seconds)}</strong>
                  </div>
                </span>
                <span>
                  <Gauge size={16} />
                  <div>
                    <small>Velocidad media</small>
                    <strong>
                      {valueWithUnit(trip.average_speed_kmh, "km/h")}
                    </strong>
                  </div>
                </span>
                <span>
                  <Gauge size={16} />
                  <div>
                    <small>Máxima GPS</small>
                    <strong>
                      {valueWithUnit(trip.maximum_speed_kmh, "km/h")}
                    </strong>
                  </div>
                </span>
              </div>
              <footer>
                <span>
                  {trip.point_count} puntos GPS ·{" "}
                  {trip.simulated ? "Datos simulados" : "Real/no simulado"}
                </span>
                <Link className="text-link" to={`/trips/${trip.id}`}>
                  Ver recorrido
                </Link>
              </footer>
            </article>
          ))}
        </div>
      )}
    </div>
  );
}
