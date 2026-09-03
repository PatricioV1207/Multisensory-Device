import {
  Bar,
  BarChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { formatChartTick } from "./chartData";

export interface FleetMetricPoint {
  label: string;
  value: number | null;
}

export function FleetMetricChart({
  data,
  color,
  unit,
  label,
}: {
  data: FleetMetricPoint[];
  color: string;
  unit: string;
  label: string;
}) {
  const maximum = Math.max(
    1,
    ...data.flatMap((point) =>
      point.value !== null && Number.isFinite(point.value) ? [point.value] : [],
    ),
  );

  return (
    <div className="chart" aria-label={`${label} por vehículo`}>
      <ResponsiveContainer width="100%" height="100%">
        <BarChart
          data={data}
          margin={{ top: 8, right: 8, left: -22, bottom: 0 }}
        >
          <CartesianGrid
            stroke="#edf2f7"
            strokeDasharray="3 3"
            vertical={false}
          />
          <XAxis
            dataKey="label"
            tick={{ fontSize: 10 }}
            tickLine={false}
            axisLine={false}
          />
          <YAxis
            domain={[0, maximum * 1.08]}
            tickFormatter={formatChartTick}
            tick={{ fontSize: 10 }}
            tickLine={false}
            axisLine={false}
          />
          <Tooltip
            formatter={(value) =>
              typeof value === "number"
                ? [`${value.toFixed(1)} ${unit}`, label]
                : ["—", label]
            }
            contentStyle={{
              borderRadius: 12,
              border: "1px solid #e5eaf1",
              fontSize: 12,
            }}
          />
          <Bar
            dataKey="value"
            name={label}
            fill={color}
            fillOpacity={0.82}
            radius={[5, 5, 0, 0]}
            maxBarSize={28}
            isAnimationActive={false}
          />
        </BarChart>
      </ResponsiveContainer>
    </div>
  );
}
