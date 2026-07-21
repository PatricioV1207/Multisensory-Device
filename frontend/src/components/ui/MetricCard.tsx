import type { LucideIcon } from "lucide-react";
import type { ReactNode } from "react";

interface MetricCardProps {
  label: string;
  value: ReactNode;
  hint?: string;
  icon: LucideIcon;
  tone?: "blue" | "green" | "teal" | "orange" | "red" | "cyan" | "purple";
  compact?: boolean;
}

export function MetricCard({
  label,
  value,
  hint,
  icon: Icon,
  tone = "blue",
  compact = false,
}: MetricCardProps) {
  return (
    <article className={`metric-card ${compact ? "metric-card--compact" : ""}`}>
      <span className={`metric-card__icon metric-card__icon--${tone}`}>
        <Icon size={compact ? 21 : 24} strokeWidth={2} aria-hidden="true" />
      </span>
      <div className="metric-card__content">
        <span className="metric-card__label">{label}</span>
        <strong className="metric-card__value">{value}</strong>
        {hint && <span className="metric-card__hint">{hint}</span>}
      </div>
    </article>
  );
}
