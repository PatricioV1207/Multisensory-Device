import "@testing-library/jest-dom/vitest";
import { createElement, type ReactNode } from "react";
import { afterEach, vi } from "vitest";
import { cleanup } from "@testing-library/react";

afterEach(() => cleanup());

class ResizeObserverStub {
  observe() {}
  unobserve() {}
  disconnect() {}
}

vi.stubGlobal("ResizeObserver", ResizeObserverStub);
Object.defineProperty(window, "matchMedia", {
  writable: true,
  value: vi.fn().mockImplementation((query: string) => ({
    matches: false,
    media: query,
    onchange: null,
    addListener: vi.fn(),
    removeListener: vi.fn(),
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    dispatchEvent: vi.fn(),
  })),
});

vi.mock("react-leaflet", () => ({
  MapContainer: ({ children }: { children: ReactNode }) =>
    createElement("div", { "data-testid": "map" }, children),
  TileLayer: () => null,
  CircleMarker: ({ children }: { children?: ReactNode }) =>
    createElement("span", null, children),
  Popup: ({ children }: { children?: ReactNode }) =>
    createElement("span", null, children),
  Polyline: () => null,
}));

vi.mock("recharts", () => {
  const container = ({ children }: { children?: ReactNode }) =>
    createElement("div", { "data-testid": "chart" }, children);
  const empty = () => null;
  return {
    ResponsiveContainer: container,
    LineChart: container,
    PieChart: container,
    Line: empty,
    Pie: container,
    Cell: empty,
    CartesianGrid: empty,
    Tooltip: empty,
    XAxis: empty,
    YAxis: empty,
  };
});
