# VehicleSense frontend

Aplicación React responsive para supervisión de flota, telemetría, GPS,
acústica, alertas y viajes. Sigue el lenguaje visual de las referencias del
proyecto: interfaz clara, tarjetas redondeadas, azul/cian/teal, estados verdes,
advertencias amarillas y alertas rojas.

## Principios de datos

- REST `/api/v1` carga identidad, estado inicial e histórico.
- WebSocket `/ws/v1/live` invalida las consultas afectadas por cambios en vivo.
- Una reconexión siempre refresca REST; WebSocket no se trata como historial.
- El navegador no se conecta a HiveMQ ni contiene credenciales MQTT.
- `null`, `NaN`, sensor inválido y ausencia de fix se muestran como `—`,
  `No válido` o `Sin GPS`; solo se muestra cero cuando es una medición válida.
- `simulated` y `replayed` se conservan y el modo demo se anuncia en pantalla.

## Stack

- React 19 + TypeScript + Vite;
- React Router y TanStack Query;
- Leaflet + OpenStreetMap;
- Recharts;
- Vitest + Testing Library.

Las rutas usan carga diferida. Mapas y gráficos se empaquetan en chunks
separados para reducir el JavaScript inicial.

## Desarrollo

```bash
cd frontend
cp .env.example .env
npm ci
npm run dev
```

Con Vite en `127.0.0.1:5173`, `/api`, `/health` y `/ws` se redirigen al backend
local en `127.0.0.1:8000`. En despliegue, Nginx sirve el frontend y redirige
estas rutas al contenedor FastAPI.

Variables disponibles:

| Variable            | Uso                                                    |
| ------------------- | ------------------------------------------------------ |
| `VITE_API_BASE_URL` | Origen REST si no se usa el mismo host                 |
| `VITE_WS_BASE_URL`  | Origen `ws://`/`wss://` si no se usa el mismo host     |
| `VITE_DEMO_MODE`    | Habilita exclusivamente el adaptador sintético marcado |

No debe definirse ninguna variable HiveMQ, PostgreSQL o clave administrativa
en Vite: todo `VITE_*` queda visible dentro del navegador.

## Demostración sin hardware

```bash
VITE_DEMO_MODE=true npm run dev
```

La pantalla de acceso ofrece **Entrar a la demostración**. Las tres unidades,
posiciones, muestras, alertas y viajes son sintéticos y se marcan con un banner
persistente. El modo no se activa como fallback cuando el backend falla.

Para probar la cadena completa sin ESP32 se usará el simulador MQTT del
repositorio, manteniendo `VITE_DEMO_MODE=false`; de ese modo el frontend consume
datos simulados que realmente pasaron por HiveMQ, validación y PostgreSQL.

## Páginas

- `/`: resumen, mapa, alertas, vehículos, viajes y analítica instantánea.
- `/map`: posiciones GPS válidas de toda la flota.
- `/vehicles`: búsqueda e inventario.
- `/vehicles/:vehicle_id`: ruta, telemetría, salud, acústica e histórico.
- `/alerts`: filtros y acciones reconocer/resolver según rol.
- `/trips`: viajes GPS inferidos, distancia y velocidades.
- `/analytics`: series de 24 horas por vehículo.
- `/reports`: exportaciones CSV y JSON de la ventana consultada.
- `/devices`: firmware, asignación y conectividad.
- `/settings`: sesión, transporte y límites de seguridad.

El detalle técnico completo permanece en un bloque expandible. Las pantallas
primarias no muestran combustible, batería, OBD, conductor o 4G porque esas
fuentes no existen en el hardware actual.

## Sesión y tiempo real

Los access/refresh JWT se mantienen en `sessionStorage`. Un `401` intenta una
renovación una sola vez; si falla, elimina la sesión. El WebSocket envía el
access token en su primer frame de autenticación, nunca como query param, y
reconecta con backoff entre 1 y 30 segundos. Los estados visibles son
conectado, conectando, reconectando, no disponible y demo.

En producción deben aplicarse HTTPS, CSP, `X-Frame-Options`, `nosniff` y demás
cabeceras en Nginx. La sesión en navegador no sustituye controles de rol del
backend.

## Calidad

```bash
npm test
npm run lint
npm run build
```

Las pruebas cubren:

- acceso demo explícito y dashboard;
- sensor/GPS inválido sin ceros ficticios;
- reconocimiento de alertas;
- navegación responsive;
- formato de valores y magnitud vectorial.

Además se validaron manualmente dashboard, detalle, menú móvil y flujo de
alertas a 1680×945, 1440×900 y 390×844. En esos tamaños no existe overflow
horizontal ni errores de consola.

## Límites

- La cartografía usa tiles públicos de OpenStreetMap; una instalación de gran
  escala debe revisar su política de uso o contratar un proveedor de tiles.
- Las gráficas consultan ventanas acotadas por la API y no hacen agregación
  estadística de largo plazo todavía.
- Los valores acústicos son dBFS relativos; no son dB SPL calibrados.
- La clasificación acústica heurística sigue marcada como no validada en campo.
