import type { LucideIcon } from "lucide-react";
import type { ReactNode } from "react";

export function DataValue({
  icon: Icon,
  label,
  value,
  valid = true,
}: {
  icon: LucideIcon;
  label: string;
  value: ReactNode;
  valid?: boolean;
}) {
  return (
    <div className={`data-value ${valid ? "" : "data-value--invalid"}`}>
      <Icon size={17} aria-hidden="true" />
      <div>
        <span>{label}</span>
        <strong>{valid ? value : "No válido"}</strong>
      </div>
    </div>
  );
}
