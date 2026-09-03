import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import {
  chartDomain,
  formatChartTick,
  prepareChartData,
  type ChartPoint,
  type SmoothingMode,
} from "./chartData";

export type { ChartPoint } from "./chartData";

export function TelemetryChart({
  data,
  color = "#087cf0",
  unit,
  label,
  smoothing = "none",
  smoothingWindow = 5,
  includeZero = false,
}: {
  data: ChartPoint[];
  color?: string;
  unit: string;
  label: string;
  smoothing?: SmoothingMode;
  smoothingWindow?: number;
  includeZero?: boolean;
}) {
  const chartData = prepareChartData(data, smoothing, smoothingWindow);
  const domain = chartDomain(data, includeZero);
  const showsTrend = smoothing !== "none";

  return (
    <div
      className={`chart${showsTrend ? " chart--smoothed" : ""}`}
      aria-label={
        showsTrend ? `${label}: medición original y tendencia suavizada` : label
      }
    >
      {showsTrend && (
        <div className="chart-key" aria-hidden="true">
          <span className="chart-key__raw">Medición</span>
          <span className="chart-key__trend">Tendencia</span>
        </div>
      )}
      <ResponsiveContainer width="100%" height="100%">
        <LineChart
          data={chartData}
          margin={{ top: showsTrend ? 26 : 8, right: 8, left: -22, bottom: 0 }}
        >
          <CartesianGrid
            stroke="#edf2f7"
            strokeDasharray="3 3"
            vertical={false}
          />
          <XAxis
            dataKey="time"
            tick={{ fontSize: 10 }}
            tickLine={false}
            axisLine={false}
          />
          <YAxis
            domain={domain}
            tickFormatter={formatChartTick}
            tick={{ fontSize: 10 }}
            tickLine={false}
            axisLine={false}
          />
          <Tooltip
            formatter={(value, name) =>
              typeof value === "number"
                ? [`${value.toFixed(1)} ${unit}`, name]
                : ["—", name]
            }
            contentStyle={{
              borderRadius: 12,
              border: "1px solid #e5eaf1",
              fontSize: 12,
            }}
          />
          {showsTrend ? (
            <>
              <Line
                type="linear"
                dataKey="rawValue"
                name={`${label} · medición`}
                stroke={color}
                strokeOpacity={0.26}
                strokeWidth={1.2}
                dot={false}
                connectNulls={false}
                isAnimationActive={false}
              />
              <Line
                type="monotone"
                dataKey="trendValue"
                name={`${label} · tendencia`}
                stroke={color}
                strokeWidth={2.4}
                dot={false}
                connectNulls={false}
                isAnimationActive={false}
              />
            </>
          ) : (
            <Line
              type="monotone"
              dataKey="value"
              name={label}
              stroke={color}
              strokeWidth={2.2}
              dot={false}
              connectNulls={false}
              isAnimationActive={false}
            />
          )}
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
