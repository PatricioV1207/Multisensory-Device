import { CloudOff, FlaskConical, LoaderCircle, Radio } from "lucide-react";
import type { LiveConnectionState } from "../../hooks/useLiveUpdates";

const labels: Record<LiveConnectionState, string> = {
  connected: "Datos en vivo",
  connecting: "Conectando en vivo",
  reconnecting: "Reconectando",
  offline: "Canal en vivo no disponible",
  demo: "Demostración simulada",
};

export function LiveIndicator({ state }: { state: LiveConnectionState }) {
  const Icon =
    state === "demo"
      ? FlaskConical
      : state === "offline"
        ? CloudOff
        : state === "connected"
          ? Radio
          : LoaderCircle;
  return (
    <span className={`live-indicator live-indicator--${state}`}>
      <Icon size={14} className={state === "connecting" ? "spin" : undefined} />
      {labels[state]}
    </span>
  );
}
