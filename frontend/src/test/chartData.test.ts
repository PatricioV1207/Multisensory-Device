import { describe, expect, it } from "vitest";
import {
  chartDomain,
  formatChartTick,
  prepareChartData,
  type ChartPoint,
} from "../components/charts/chartData";

function points(values: Array<number | null>): ChartPoint[] {
  return values.map((value, index) => ({ time: String(index), value }));
}

describe("telemetry chart preparation", () => {
  it("uses a centered moving average without changing segment boundaries", () => {
    const prepared = prepareChartData(
      points([0, 3, 9, 3, 0, null, 10, 20, 30]),
      "mean",
      3,
    );

    expect(prepared.map((point) => point.trendValue)).toEqual([
      0,
      4,
      5,
      4,
      0,
      null,
      10,
      20,
      30,
    ]);
    expect(prepared.map((point) => point.rawValue)).toEqual([
      0,
      3,
      9,
      3,
      0,
      null,
      10,
      20,
      30,
    ]);
  });

  it("uses a short median filter for spikes while preserving endpoints", () => {
    const prepared = prepareChartData(points([0, 100, 10, 12, 0]), "median", 3);

    expect(prepared.map((point) => point.trendValue)).toEqual([
      0, 10, 12, 10, 0,
    ]);
  });

  it("keeps invalid values as gaps and excludes them from the axis domain", () => {
    const data = points([null, Number.NaN, 1006, 1008]);
    const prepared = prepareChartData(data, "mean", 5);

    expect(prepared.map((point) => point.trendValue)).toEqual([
      null,
      null,
      1006,
      1008,
    ]);
    expect(chartDomain(data, false)).toEqual([1005.76, 1008.24]);
  });

  it("retains a zero baseline where zero has semantic meaning", () => {
    expect(chartDomain(points([0, 20, 40]), true)).toEqual([0, 43.2]);
  });

  it("formats calculated axis ticks without long floating-point tails", () => {
    expect(formatChartTick(1005.1241457000983)).toBe("1.005,1");
    expect(formatChartTick(Number.NaN)).toBe("—");
  });
});
