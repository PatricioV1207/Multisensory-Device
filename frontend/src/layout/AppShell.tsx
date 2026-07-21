import {
  Activity,
  Bell,
  BusFront,
  CalendarDays,
  ChartNoAxesCombined,
  ChevronDown,
  FileChartColumn,
  Gauge,
  LayoutDashboard,
  MapPinned,
  Menu,
  Search,
  Settings,
  ShieldCheck,
  Truck,
  UsersRound,
  X,
} from "lucide-react";
import { useState } from "react";
import { NavLink, Outlet, useNavigate } from "react-router-dom";
import { useAuth } from "../auth/AuthContext";
import {
  useLiveUpdates,
  type LiveConnectionState,
} from "../hooks/useLiveUpdates";
import { isDemoMode } from "../lib/api";

const navigation = [
  { to: "/", label: "Dashboard", icon: LayoutDashboard, end: true },
  { to: "/map", label: "Mapa en vivo", icon: MapPinned },
  { to: "/vehicles", label: "Vehículos", icon: Truck },
  { to: "/alerts", label: "Alertas", icon: Bell },
  { to: "/trips", label: "Viajes", icon: BusFront },
  { to: "/analytics", label: "Analítica", icon: ChartNoAxesCombined },
  { to: "/reports", label: "Reportes", icon: FileChartColumn },
  { to: "/devices", label: "Dispositivos", icon: Activity },
  { to: "/settings", label: "Configuración", icon: Settings },
];

export interface ShellContext {
  liveState: LiveConnectionState;
}

export function AppShell() {
  const [menuOpen, setMenuOpen] = useState(false);
  const [search, setSearch] = useState("");
  const { user, logout } = useAuth();
  const navigate = useNavigate();
  const liveState = useLiveUpdates();
  const current = new Intl.DateTimeFormat("es-EC", {
    dateStyle: "medium",
    timeStyle: "short",
  }).format(new Date());

  const submitSearch = (event: React.FormEvent) => {
    event.preventDefault();
    const query = search.trim();
    navigate(
      query ? `/vehicles?search=${encodeURIComponent(query)}` : "/vehicles",
    );
    setMenuOpen(false);
  };

  return (
    <div className="app-shell">
      <aside className={`sidebar ${menuOpen ? "sidebar--open" : ""}`}>
        <div className="brand">
          <span className="brand__mark">
            <Truck size={23} />
          </span>
          <span>
            Vehicle<strong>Sense</strong>
          </span>
          <button
            type="button"
            className="icon-button sidebar__close"
            aria-label="Cerrar navegación"
            onClick={() => setMenuOpen(false)}
          >
            <X size={20} />
          </button>
        </div>
        <nav className="sidebar__nav" aria-label="Navegación principal">
          {navigation.map(({ to, label, icon: Icon, end }) => (
            <NavLink
              key={to}
              to={to}
              end={end}
              onClick={() => setMenuOpen(false)}
              className={({ isActive }) =>
                `nav-link ${isActive ? "nav-link--active" : ""}`
              }
            >
              <Icon size={19} />
              <span>{label}</span>
            </NavLink>
          ))}
        </nav>
        <div className="sidebar__health">
          <ShieldCheck size={24} />
          <div>
            <strong>Supervisión activa</strong>
            <span>Estados y alertas operativos</span>
          </div>
        </div>
      </aside>

      {menuOpen && (
        <button
          type="button"
          aria-label="Cerrar navegación por fondo"
          className="sidebar-backdrop"
          onClick={() => setMenuOpen(false)}
        />
      )}

      <div className="workspace">
        <header className="topbar">
          <button
            type="button"
            className="icon-button topbar__menu"
            aria-label="Abrir navegación"
            onClick={() => setMenuOpen(true)}
          >
            <Menu size={21} />
          </button>
          <form className="search" role="search" onSubmit={submitSearch}>
            <Search size={18} />
            <input
              value={search}
              onChange={(event) => setSearch(event.target.value)}
              placeholder="Buscar vehículo o dispositivo…"
              aria-label="Buscar vehículo o dispositivo"
            />
            <kbd>⌘ K</kbd>
          </form>
          <div className="topbar__date">
            <CalendarDays size={18} />
            <span>{current}</span>
          </div>
          <button
            className="topbar__notifications"
            type="button"
            aria-label="Ver alertas"
            onClick={() => navigate("/alerts")}
          >
            <Bell size={20} />
          </button>
          <div className="profile">
            <span className="profile__avatar">
              <UsersRound size={18} />
            </span>
            <div>
              <strong>{user?.email.split("@")[0] ?? "Usuario"}</strong>
              <span>{user?.role ?? "viewer"}</span>
            </div>
            <button
              className="icon-button"
              onClick={logout}
              type="button"
              aria-label="Cerrar sesión"
              title="Cerrar sesión"
            >
              <ChevronDown size={17} />
            </button>
          </div>
        </header>
        {isDemoMode && (
          <div className="demo-banner" role="status">
            <Gauge size={15} /> Modo demostración: todos los vehículos y
            mediciones visibles son simulados.
          </div>
        )}
        <main className="main-content">
          <Outlet context={{ liveState } satisfies ShellContext} />
        </main>
      </div>
    </div>
  );
}
