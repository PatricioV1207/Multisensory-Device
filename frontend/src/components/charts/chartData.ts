export interface ChartPoint {
  time: string;
  value: number | null;
}

export type SmoothingMode = "mean" | "median" | "none";

export interface PreparedChartPoint extends ChartPoint {
  rawValue: number | null;
  trendValue: number | null;
}

export function formatChartTick(value: number): string {
  return Number.isFinite(value)
    ? value.toLocaleString("es-EC", { maximumFractionDigits: 1 })
    : "—";
}

function finiteValue(value: number | null): number | null {
  return typeof value === "number" && Number.isFinite(value) ? value : null;
}

function median(values: number[]): number {
  const sorted = [...values].sort((left, right) => left - right);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2
    ? sorted[middle]
    : (sorted[middle - 1] + sorted[middle]) / 2;
}

export function prepareChartData(
  data: ChartPoint[],
  smoothing: SmoothingMode,
  requestedWindowSize: number,
): PreparedChartPoint[] {
  const values = data.map((point) => finiteValue(point.value));
  const windowSize = Math.max(1, Math.floor(requestedWindowSize));
  const oddWindowSize = windowSize % 2 ? windowSize : windowSize + 1;
  const radius = Math.floor(oddWindowSize / 2);

  return data.map((point, index) => {
    const rawValue = values[index];
    let trendValue = rawValue;

    if (smoothing !== "none" && rawValue !== null && radius > 0) {
      const start = index - radius;
      const end = index + radius;
      const window = values.slice(start, end + 1);

      // Only smooth complete, contiguous windows. This preserves invalid-data
      // gaps and leaves segment boundaries faithful to the original samples.
      if (
        start >= 0 &&
        end < values.length &&
        window.length === oddWindowSize &&
        window.every((value): value is number => value !== null)
      ) {
        trendValue =
          smoothing === "median"
            ? median(window)
            : window.reduce((sum, value) => sum + value, 0) / window.length;
      }
    }

    return {
      ...point,
      value: rawValue,
      rawValue,
      trendValue,
    };
  });
}

export function chartDomain(
  data: ChartPoint[],
  includeZero: boolean,
): [number, number] | undefined {
  const values = data
    .map((point) => finiteValue(point.value))
    .filter((value): value is number => value !== null);
  if (!values.length) return undefined;

  const minimum = Math.min(...values);
  const maximum = Math.max(...values);
  if (includeZero) {
    return [0, Math.max(1, maximum * 1.08)];
  }

  const span = maximum - minimum;
  const padding =
    span > 0 ? span * 0.12 : Math.max(Math.abs(minimum) * 0.02, 1);
  return [minimum - padding, maximum + padding];
}
