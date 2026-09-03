import {
  ArrowLeft,
  Clock3,
  Gauge,
  MapPin,
  Route,
  Satellite,
} from "lucide-react";
import { Link, useParams } from "react-router-dom";
import { RouteMap } from "../components/maps/VehicleMap";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { MetricCard } from "../components/ui/MetricCard";
import { Panel } from "../components/ui/Panel";
import { useTrip } from "../hooks/queries";
import {
  compactDuration,
  formatDateTime,
  numberOrDash,
  tripStateLabels,
  valueWithUnit,
} from "../lib/format";

function coordinates(value: [number, number] | null): string {
  return value ? `${value[0].toFixed(5)}, ${value[1].toFixed(5)}` : "Sin datos";
}

function gpsQuality(hdop: number | null): string {
  if (hdop == null) return "Sin datos";
  if (hdop <= 1) return "Excelente";
  if (hdop <= 2) return "Buena";
  if (hdop <= 5) return "Moderada";
  return "Débil";
}

export function TripDetailPage() {
  const { tripId = "" } = useParams();
  const trip = useTrip(tripId);
  if (trip.isLoading) return <PageLoader label="Cargando recorrido…" />;
  if (trip.isError || !trip.data) {
    return (
      <ErrorState
        title="No se encontró el viaje"
        description="Comprueba el identificador o vuelve al historial semanal."
        retry={() => void trip.refetch()}
      />
    );
  }
  const detail = trip.data;
  return (
    <div className="page trip-detail-page">
      <header className="vehicle-heading">
        <div>
          <Link className="back-link" to="/trips">
            <ArrowLeft size={15} /> Viajes
          </Link>
          <div className="vehicle-heading__title">
            <h1>Detalle del viaje</h1>
            <span className={`trip-state trip-state--${detail.status}`}>
              {tripStateLabels[detail.status]}
            </span>
          </div>
          <p>
            {detail.vehicle_id ?? "Sin vehículo"} · Inicio{" "}
            {formatDateTime(detail.started_at)}
          </p>
        </div>
        {detail.vehicle_id && (
          <Link
            className="button button--secondary"
            to={`/vehicles/${detail.vehicle_id}`}
          >
            Ver vehículo
          </Link>
        )}
      </header>

      <section className="metric-grid metric-grid--four">
        <MetricCard
          compact
          label="Distancia"
          value={valueWithUnit(detail.distance_km, "km")}
          hint="Calculada con la ruta GPS"
          icon={Route}
          tone="blue"
        />
        <MetricCard
          compact
          label="Duración"
          value={compactDuration(detail.duration_seconds)}
          hint={
            detail.ended_at
              ? `Fin ${formatDateTime(detail.ended_at)}`
              : "Viaje activo"
          }
          icon={Clock3}
          tone="teal"
        />
        <MetricCard
          compact
          label="Velocidad media"
          value={valueWithUnit(detail.average_speed_kmh, "km/h")}
          hint="Media de muestras GPS"
          icon={Gauge}
          tone="green"
        />
        <MetricCard
          compact
          label="Máxima GPS"
          value={valueWithUnit(detail.maximum_speed_kmh, "km/h")}
          hint={
            detail.maximum_speed_at
              ? formatDateTime(detail.maximum_speed_at)
              : "Sin datos"
          }
          icon={Gauge}
          tone="orange"
        />
      </section>

      <Panel
        className="map-panel trip-route-panel"
        title="Ruta del viaje"
        eyebrow={`${detail.point_count} puntos GPS guardados`}
      >
        {detail.points.length ? (
          <RouteMap
            points={detail.points}
            title={detail.vehicle_id ?? "Viaje"}
          />
        ) : (
          <EmptyState
            title="Este viaje no tiene puntos de ruta"
            description="El resumen existe, pero no se registraron posiciones GPS utilizables."
          />
        )}
      </Panel>

      <div className="trip-detail-grid">
        <Panel title="Origen y destino" eyebrow="Coordenadas registradas">
          <dl className="trip-detail-list">
            <div>
              <dt>
                <MapPin size={15} /> Inicio
              </dt>
              <dd>{coordinates(detail.start)}</dd>
            </div>
            <div>
              <dt>
                <MapPin size={15} /> Final
              </dt>
              <dd>
                {coordinates(
                  detail.end ??
                    (detail.points.length
                      ? [
                          detail.points.at(-1)!.latitude,
                          detail.points.at(-1)!.longitude,
                        ]
                      : null),
                )}
              </dd>
            </div>
          </dl>
        </Panel>
        <Panel
          title="Calidad del recorrido"
          eyebrow="Datos directamente observados"
        >
          <dl className="trip-detail-list">
            <div>
              <dt>
                <Satellite size={15} /> HDOP medio
              </dt>
              <dd>
                {numberOrDash(detail.average_gps_hdop, 2)} ·{" "}
                {gpsQuality(detail.average_gps_hdop)}
              </dd>
            </div>
            <div>
              <dt>
                <Route size={15} /> Cobertura
              </dt>
              <dd>{detail.points.length} posiciones en la ruta</dd>
            </div>
          </dl>
        </Panel>
      </div>

      <p className="information-note">
        El viaje se infiere por movimiento GPS. No indica encendido, conductor,
        combustible ni diagnóstico del vehículo.{" "}
        {detail.simulated
          ? "Estos datos son simulados."
          : "Datos reales/no simulados."}
      </p>
    </div>
  );
}
