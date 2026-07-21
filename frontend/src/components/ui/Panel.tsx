import type { PropsWithChildren, ReactNode } from "react";

interface PanelProps extends PropsWithChildren {
  title?: string;
  eyebrow?: string;
  action?: ReactNode;
  className?: string;
}

export function Panel({
  title,
  eyebrow,
  action,
  className = "",
  children,
}: PanelProps) {
  return (
    <section className={`panel ${className}`}>
      {(title || action) && (
        <header className="panel__header">
          <div>
            {eyebrow && <span className="panel__eyebrow">{eyebrow}</span>}
            {title && <h2>{title}</h2>}
          </div>
          {action && <div className="panel__action">{action}</div>}
        </header>
      )}
      {children}
    </section>
  );
}
