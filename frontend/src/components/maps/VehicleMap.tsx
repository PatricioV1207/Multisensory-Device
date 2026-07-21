import {
  CircleMarker,
  MapContainer,
  Polyline,
  Popup,
  TileLayer,
} from "react-leaflet";
import type { Position, RoutePoint } from "../../types/api";
import { valueWithUnit } from "../../lib/format";

const DEFAULT_CENTER: [number, number] = [-2.1709, -79.9224];

export function FleetMap({ positions }: { positions: Position[] }) {
  const center: [number, number] = positions.length
    ? [positions[0].latitude, positions[0].longitude]
    : DEFAULT_CENTER;
  return (
    <MapContainer
      center={center}
      zoom={13}
      scrollWheelZoom
      className="map-canvas"
    >
      <TileLayer
        attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      {positions.map((position) => (
        <CircleMarker
          key={position.vehicle_id}
          center={[position.latitude, position.longitude]}
          radius={11}
          pathOptions={{
            fillColor: position.status === "online" ? "#0bbf83" : "#f2a900",
            fillOpacity: 1,
            color: "#ffffff",
            weight: 4,
          }}
        >
          <Popup>
            <strong>{position.display_name}</strong>
            <br />
            {valueWithUnit(position.speed_kmh, "km/h", 0)}
          </Popup>
        </CircleMarker>
      ))}
    </MapContainer>
  );
}

export function RouteMap({
  points,
  title,
}: {
  points: RoutePoint[];
  title: string;
}) {
  const coordinates = points.map(
    (point) => [point.latitude, point.longitude] as [number, number],
  );
  const center = coordinates.at(-1) ?? DEFAULT_CENTER;
  return (
    <MapContainer
      center={center}
      zoom={14}
      scrollWheelZoom
      className="map-canvas"
    >
      <TileLayer
        attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      {coordinates.length > 1 && (
        <Polyline
          positions={coordinates}
          pathOptions={{ color: "#087cf0", weight: 5 }}
        />
      )}
      {coordinates.length > 0 && (
        <>
          <CircleMarker
            center={coordinates[0]}
            radius={7}
            pathOptions={{
              fillColor: "#16b876",
              fillOpacity: 1,
              color: "white",
              weight: 3,
            }}
          >
            <Popup>Inicio de la ruta visible</Popup>
          </CircleMarker>
          <CircleMarker
            center={coordinates.at(-1)!}
            radius={10}
            pathOptions={{
              fillColor: "#087cf0",
              fillOpacity: 1,
              color: "white",
              weight: 4,
            }}
          >
            <Popup>{title} · última posición</Popup>
          </CircleMarker>
        </>
      )}
    </MapContainer>
  );
}
