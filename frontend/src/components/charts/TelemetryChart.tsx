import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export interface ChartPoint {
  time: string;
  value: number | null;
}

export function TelemetryChart({
  data,
  color = "#087cf0",
  unit,
  label,
}: {
  data: ChartPoint[];
  color?: string;
  unit: string;
  label: string;
}) {
  return (
    <div className="chart" aria-label={label}>
      <ResponsiveContainer width="100%" height="100%">
        <LineChart
          data={data}
          margin={{ top: 8, right: 8, left: -22, bottom: 0 }}
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
          <YAxis tick={{ fontSize: 10 }} tickLine={false} axisLine={false} />
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
          <Line
            type="monotone"
            dataKey="value"
            stroke={color}
            strokeWidth={2.2}
            dot={false}
            connectNulls={false}
            isAnimationActive={false}
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
