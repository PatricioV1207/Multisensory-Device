import {
  KeyRound,
  Radio,
  Settings2,
  ShieldCheck,
  UsersRound,
} from "lucide-react";
import { useAuth } from "../auth/AuthContext";
import { Panel } from "../components/ui/Panel";
import { isDemoMode } from "../lib/api";

export function SettingsPage() {
  const { user } = useAuth();
  return (
    <div className="page">
      <header className="page-heading">
        <div>
          <span className="eyebrow">Administración</span>
          <h1>Configuración</h1>
          <p>Resumen de sesión, transporte y límites operativos.</p>
        </div>
      </header>
      <div className="settings-grid">
        <Panel title="Sesión actual">
          <dl className="settings-list">
            <div>
              <dt>
                <UsersRound size={17} /> Usuario
              </dt>
              <dd>{user?.email ?? "—"}</dd>
            </div>
            <div>
              <dt>
                <ShieldCheck size={17} /> Rol
              </dt>
              <dd>{user?.role ?? "—"}</dd>
            </div>
            <div>
              <dt>
                <KeyRound size={17} /> Autenticación
              </dt>
              <dd>JWT access + refresh</dd>
            </div>
          </dl>
        </Panel>
        <Panel title="Fuente de datos">
          <dl className="settings-list">
            <div>
              <dt>
                <Radio size={17} /> Modo
              </dt>
              <dd>
                {isDemoMode ? "Demostración simulada" : "Backend VehicleSense"}
              </dd>
            </div>
            <div>
              <dt>
                <Settings2 size={17} /> Historial
              </dt>
              <dd>REST `/api/v1`</dd>
            </div>
            <div>
              <dt>
                <Radio size={17} /> Datos en vivo
              </dt>
              <dd>WebSocket FastAPI</dd>
            </div>
          </dl>
        </Panel>
      </div>
      <Panel title="Configuración operativa">
        <div className="information-note">
          <p>
            Los umbrales por vehículo, usuarios y asignaciones se gestionan
            mediante endpoints protegidos del backend. Este primer cliente
            muestra el estado efectivo sin exponer credenciales HiveMQ ni
            secretos de infraestructura en el navegador.
          </p>
        </div>
      </Panel>
      <Panel title="Seguridad">
        <ul className="plain-list">
          <li>
            HiveMQ solo es accesible desde dispositivos y backend mediante ACL
            específicas.
          </li>
          <li>PostgreSQL no se expone al navegador.</li>
          <li>
            Los tokens se conservan en `sessionStorage` y se eliminan al cerrar
            sesión.
          </li>
          <li>
            Producción debe servirse únicamente por HTTPS detrás de Nginx.
          </li>
        </ul>
      </Panel>
    </div>
  );
}
