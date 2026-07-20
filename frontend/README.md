# VehicleSense frontend

Estado: límites de producto y referencia visual definidos; implementación
prevista para la fase 6.

## Fuente de datos

- REST `/api/v1` para identidad, histórico, alertas, viajes y configuración;
- WebSocket autenticado para cambios recientes;
- nunca conexión MQTT directa;
- nunca credenciales HiveMQ en el bundle del navegador.

## Stack acordado

- React + TypeScript + Vite;
- React Router;
- TanStack Query;
- Leaflet + OpenStreetMap;
- Recharts;
- Vitest + Testing Library;
- Playwright para flujos end-to-end.

## Áreas

- Dashboard de flota.
- Mapa en vivo.
- Vehículos y detalle de vehículo.
- Alertas.
- Viajes.
- Analítica.
- Reportes/exportaciones.
- Dispositivos.
- Configuración.

Las dos imágenes suministradas en la tarea son la referencia principal para el
dashboard general y el detalle. Se usará su lenguaje visual claro, azul/cian/
teal, tarjetas redondeadas y jerarquía compacta. No se copiarán sus valores
ficticios ni se presentarán 4G, combustible, conductor u OBD como datos reales.

La interfaz distinguirá explícitamente loading, vacío, sin GPS, sensor
inválido, offline, stale, backend caído, WebSocket reconectando y permiso
insuficiente. Un dato ausente no se representa como cero.
