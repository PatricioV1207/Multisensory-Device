import {
  AlertTriangle,
  Check,
  CheckCircle2,
  Filter,
  ShieldAlert,
} from "lucide-react";
import { useState } from "react";
import { Link } from "react-router-dom";
import {
  EmptyState,
  ErrorState,
  PageLoader,
} from "../components/ui/AsyncState";
import { SeverityBadge } from "../components/ui/StatusBadge";
import { useAlertAction, useAlerts } from "../hooks/queries";
import {
  alertStateLabels,
  alertTypeLabel,
  formatDateTime,
} from "../lib/format";
import type { AlertState } from "../types/api";

export function AlertsPage() {
  const [filter, setFilter] = useState<AlertState | "all">("all");
  const alerts = useAlerts();
  const action = useAlertAction();
  if (alerts.isLoading) return <PageLoader label="Cargando alertas…" />;
  if (alerts.isError) return <ErrorState retry={() => void alerts.refetch()} />;
  const filtered = (alerts.data ?? []).filter(
    (alert) => filter === "all" || alert.status === filter,
  );
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Eventos</span>
          <h1>Alertas</h1>
          <p>Revisa, reconoce y resuelve condiciones detectadas.</p>
        </div>
        <span className="page-count">
          <ShieldAlert size={15} /> {filtered.length}
        </span>
      </header>
      <div className="page-toolbar">
        <span className="toolbar-label">
          <Filter size={15} /> Estado
        </span>
        {(["all", "active", "acknowledged", "resolved"] as const).map(
          (item) => (
            <button
              type="button"
              key={item}
              className={`filter-chip ${filter === item ? "filter-chip--active" : ""}`}
              onClick={() => setFilter(item)}
            >
              {item === "all" ? "Todas" : alertStateLabels[item]}
            </button>
          ),
        )}
      </div>
      {!filtered.length ? (
        <EmptyState
          title="No hay alertas en este estado"
          description="Las nuevas condiciones aparecerán aquí cuando el backend las detecte."
        />
      ) : (
        <div className="alert-table-wrap">
          <table className="data-table">
            <thead>
              <tr>
                <th>Alerta</th>
                <th>Vehículo</th>
                <th>Severidad</th>
                <th>Estado</th>
                <th>Última observación</th>
                <th>Acciones</th>
              </tr>
            </thead>
            <tbody>
              {filtered.map((alert) => (
                <tr key={alert.id}>
                  <td>
                    <div className="table-primary">
                      <span
                        className={`alert-row__icon alert-row__icon--${alert.severity}`}
                      >
                        <AlertTriangle size={17} />
                      </span>
                      <div>
                        <strong>{alert.title}</strong>
                        <span>
                          {alertTypeLabel(alert.type)} ·{" "}
                          {alert.occurrence_count} observación(es)
                        </span>
                      </div>
                    </div>
                  </td>
                  <td>
                    {alert.vehicle_id ? (
                      <Link
                        className="text-link"
                        to={`/vehicles/${alert.vehicle_id}`}
                      >
                        {alert.vehicle_id}
                      </Link>
                    ) : (
                      "—"
                    )}
                  </td>
                  <td>
                    <SeverityBadge severity={alert.severity} />
                  </td>
                  <td>
                    <span className={`trip-state trip-state--${alert.status}`}>
                      {alertStateLabels[alert.status]}
                    </span>
                  </td>
                  <td>{formatDateTime(alert.last_observed_at)}</td>
                  <td>
                    <div className="table-actions">
                      {alert.status === "active" && (
                        <button
                          type="button"
                          className="button button--small button--secondary"
                          onClick={() =>
                            action.mutate({
                              id: alert.id,
                              action: "acknowledge",
                            })
                          }
                          disabled={action.isPending}
                        >
                          <Check size={14} /> Reconocer
                        </button>
                      )}
                      {alert.status !== "resolved" && (
                        <button
                          type="button"
                          className="button button--small button--ghost"
                          onClick={() =>
                            action.mutate({ id: alert.id, action: "resolve" })
                          }
                          disabled={action.isPending}
                        >
                          <CheckCircle2 size={14} /> Resolver
                        </button>
                      )}
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
