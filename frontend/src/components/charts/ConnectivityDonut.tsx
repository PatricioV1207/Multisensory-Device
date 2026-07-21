import { Cell, Pie, PieChart, ResponsiveContainer } from "recharts";

export function ConnectivityDonut({
  online,
  total,
}: {
  online: number;
  total: number;
}) {
  const safeTotal = Math.max(total, 0);
  const data = [
    { name: "En línea", value: online, color: "#16b876" },
    {
      name: "No disponible",
      value: Math.max(0, safeTotal - online),
      color: "#e9eef5",
    },
  ];
  const percent = safeTotal ? Math.round((online / safeTotal) * 100) : null;
  return (
    <div className="donut">
      <ResponsiveContainer width="100%" height="100%">
        <PieChart>
          <Pie
            data={data}
            dataKey="value"
            innerRadius="72%"
            outerRadius="93%"
            strokeWidth={0}
          >
            {data.map((item) => (
              <Cell key={item.name} fill={item.color} />
            ))}
          </Pie>
        </PieChart>
      </ResponsiveContainer>
      <div className="donut__label">
        <strong>{percent == null ? "—" : `${percent}%`}</strong>
        <span>conectividad</span>
      </div>
    </div>
  );
}
