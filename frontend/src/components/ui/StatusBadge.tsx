import {
  AlertTriangle,
  CheckCircle2,
  CircleHelp,
  Clock3,
  WifiOff,
} from "lucide-react";
import type { DeviceState, Severity } from "../../types/api";
import { severityLabels, stateLabels } from "../../lib/format";

const stateIcons = {
  online: CheckCircle2,
  stale: Clock3,
  offline: WifiOff,
  reconnecting: Clock3,
  unknown: CircleHelp,
};

export function StatusBadge({
  state,
  compact = false,
}: {
  state: DeviceState;
  compact?: boolean;
}) {
  const Icon = stateIcons[state];
  return (
    <span className={`badge badge--${state}`}>
      <Icon size={13} aria-hidden="true" />
      {compact ? stateLabels[state].split(" ")[0] : stateLabels[state]}
    </span>
  );
}

export function SeverityBadge({ severity }: { severity: Severity }) {
  return (
    <span className={`severity severity--${severity}`}>
      <AlertTriangle size={12} aria-hidden="true" />
      {severityLabels[severity]}
    </span>
  );
}

export function ValidityBadge({
  valid,
  label,
}: {
  valid: boolean;
  label: string;
}) {
  return (
    <span
      className={`sensor-pill ${valid ? "sensor-pill--valid" : "sensor-pill--invalid"}`}
    >
      <span className="sensor-pill__dot" />
      {label}
    </span>
  );
}
