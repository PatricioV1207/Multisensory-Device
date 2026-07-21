import { Clock3, Gauge, MapPin, Route } from "lucide-react";
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
  const trips = useTrips();
  if (trips.isLoading) return <PageLoader label="Cargando viajes…" />;
  if (trips.isError) return <ErrorState retry={() => void trips.refetch()} />;
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Movimiento GPS</span>
          <h1>Viajes</h1>
          <p>
            Trayectos inferidos por velocidad y posiciones válidas; no
            representan encendido.
          </p>
        </div>
        <span className="page-count">
          <Route size={15} /> {trips.data?.length ?? 0}
        </span>
      </header>
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
                  <Link
                    to={
                      trip.vehicle_id
                        ? `/vehicles/${trip.vehicle_id}`
                        : "/vehicles"
                    }
                  >
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
                {trip.point_count} puntos GPS ·{" "}
                {trip.simulated ? "Datos simulados" : "Datos de producción"}
              </footer>
            </article>
          ))}
        </div>
      )}
    </div>
  );
}
