import { AlertCircle, Inbox, LoaderCircle, RefreshCw } from "lucide-react";
import type { ReactNode } from "react";

export function PageLoader({
  label = "Cargando VehicleSense…",
}: {
  label?: string;
}) {
  return (
    <div className="async-state" role="status">
      <LoaderCircle className="spin" size={28} />
      <p>{label}</p>
    </div>
  );
}

export function EmptyState({
  title = "Sin información disponible",
  description = "Los datos aparecerán cuando un dispositivo envíe una muestra válida.",
  action,
}: {
  title?: string;
  description?: string;
  action?: ReactNode;
}) {
  return (
    <div className="async-state async-state--empty">
      <Inbox size={30} />
      <h3>{title}</h3>
      <p>{description}</p>
      {action}
    </div>
  );
}

export function ErrorState({
  title = "No pudimos obtener los datos",
  description = "Comprueba la conexión con el backend e inténtalo otra vez.",
  retry,
}: {
  title?: string;
  description?: string;
  retry?: () => void;
}) {
  return (
    <div className="async-state async-state--error" role="alert">
      <AlertCircle size={30} />
      <h3>{title}</h3>
      <p>{description}</p>
      {retry && (
        <button
          className="button button--secondary"
          onClick={retry}
          type="button"
        >
          <RefreshCw size={15} /> Reintentar
        </button>
      )}
    </div>
  );
}
