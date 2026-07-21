# VehicleSense backend

Servicio de confianza entre HiveMQ Cloud, PostgreSQL y la aplicación web. El
navegador nunca se conecta al broker ni recibe credenciales MQTT.

## Responsabilidades

- MQTT 3.1.1 sobre TLS con sesión persistente, QoS 1 y suscripciones limitadas;
- validación estricta de tópico, identidad y JSON Schema antes de persistir;
- cuarentena de mensajes incompatibles y deduplicación idempotente;
- telemetría histórica, acústica, estados, alertas, viajes y comandos;
- estados `online`, `stale`, `offline`, `reconnecting` y `unknown`;
- REST versionada, WebSocket autenticado y roles `admin`, `operator`, `viewer`;
- liveness y readiness independientes.

El stack usa Python 3.12/3.13, FastAPI, Pydantic 2, SQLAlchemy 2 async,
Alembic, PostgreSQL y Paho MQTT. Los contratos compartidos están en
[`../contracts`](../contracts/README.md).

## Puesta en marcha local

Se recomienda instalar [uv](https://docs.astral.sh/uv/) y PostgreSQL 16 o
posterior.

```bash
cd backend
cp .env.example .env
uv sync --extra dev
uv run alembic upgrade head
uv run uvicorn app.main:app --reload --host 127.0.0.1 --port 8000
```

En `.env` se deben cambiar, como mínimo, la URL PostgreSQL, el secreto JWT y
las credenciales del administrador inicial. HiveMQ puede dejarse deshabilitado
para trabajar solamente con REST, pruebas o el simulador.

La creación automática de tablas existe solo para tests. En desarrollo y
producción se usa siempre Alembic:

```bash
uv run alembic current
uv run alembic upgrade head
uv run alembic downgrade -1
```

## Configuración

Todas las variables tienen prefijo `VEHICLESENSE_`. Los secretos se leen desde
el entorno o un `.env` no versionado.

| Variable | Uso |
|---|---|
| `DATABASE_URL` | URL async SQLAlchemy, normalmente `postgresql+asyncpg://...` |
| `JWT_SECRET` | Firma de access/refresh JWT; mínimo 32 caracteres en producción |
| `BOOTSTRAP_ADMIN_EMAIL/PASSWORD` | Crea el primer administrador si no existe |
| `MQTT_ENABLED` | Habilita el consumidor HiveMQ |
| `MQTT_HOST/PORT` | Host HiveMQ y puerto TLS, normalmente 8883 |
| `MQTT_USERNAME/PASSWORD` | Credencial exclusiva del backend |
| `MQTT_CLIENT_ID` | Identificador estable y único de la sesión persistente |
| `MQTT_CA_FILE` | CA adicional opcional; vacío usa el almacén del sistema |
| `CORS_ORIGINS` | Orígenes permitidos para el frontend |
| `AUTO_REGISTER_DEVICES` | Solo para demo; en producción debe permanecer falso |
| `STALE_AFTER_SECONDS` | Ventana sin muestras para marcar `stale` |
| `OFFLINE_AFTER_SECONDS` | Ventana sin muestras para marcar `offline` |

Consulta [`.env.example`](.env.example) para la lista inicial completa.

## API y autenticación

La documentación OpenAPI está disponible en `/docs` fuera de producción.
Rutas principales:

```text
POST /api/v1/auth/login
POST /api/v1/auth/refresh
GET  /api/v1/auth/me
GET  /api/v1/dashboard
GET  /api/v1/vehicles
GET  /api/v1/vehicles/{vehicle_id}
GET  /api/v1/vehicles/{vehicle_id}/telemetry
GET  /api/v1/vehicles/{vehicle_id}/route
GET  /api/v1/vehicles/{vehicle_id}/acoustic
GET  /api/v1/devices
GET  /api/v1/alerts
PATCH /api/v1/alerts/{alert_id}
GET  /api/v1/trips
GET  /api/v1/trips/{trip_id}
POST /api/v1/commands
GET  /api/v1/commands
GET  /health/live
GET  /health/ready
```

Las consultas son accesibles a los tres roles. Crear vehículos, dispositivos y
usuarios requiere `admin`; reconocer/resolver alertas y emitir comandos admite
`admin` u `operator`.

El WebSocket se abre en `/ws/v1/live?token={access_token}`. El servidor envía
`connection.ready`; el cliente puede limitar actualizaciones con:

```json
{"action":"subscribe","vehicle_ids":["vehicle-001"]}
```

Los eventos en vivo incluyen `telemetry.received`, `acoustic.received`,
`device.status`, `alert.changed` y `command.acknowledged`. Al reconectar, el
frontend debe refrescar por REST porque WebSocket no es un historial.

## Ingesta y entrega MQTT

Los tópicos y ACL exactos están en
[`../contracts/mqtt-topics.md`](../contracts/mqtt-topics.md). El backend se
suscribe únicamente a `telemetry`, `status`, `events`, `acoustic` y
`command-acks`, y publica únicamente en `commands`.

Paho usa sesión persistente y acuse manual QoS 1. Un mensaje se confirma tras
la transacción de base de datos o después de registrar un rechazo contractual
en cuarentena. Si falla internamente la ingestión, se retiene el PUBACK y se
reconecta para solicitar la reentrega. Las claves `sample_id`, `dedupe_key`,
`event_id` y `command_id` evitan duplicados al reintentar.

## Modelo de datos

| Tabla | Propósito |
|---|---|
| `users` | credenciales Argon2, rol y actividad |
| `vehicles` | identidad, metadata y umbrales por vehículo |
| `devices` | asignación, firmware, estado y última actividad |
| `telemetry` | muestras v2/v3 normalizadas y payload original |
| `acoustic_measurements` | características agregadas, categoría y confianza |
| `alerts` | severidad, ciclo activo/reconocido/resuelto y deduplicación |
| `trips`, `trip_points` | viajes GPS inferidos y su trayectoria |
| `device_status_history` | cambios de conectividad auditables |
| `commands` | cola, publicación, expiración y acuse |
| `ingestion_failures` | mensajes rechazados sin guardar secretos de conexión |

La hora de medición y la hora de recepción se almacenan por separado. Los
registros `replayed` no alteran el detector de viajes en vivo y los registros
`simulated` permanecen identificados en toda la ruta.

## Calidad

```bash
uv run ruff format --check app tests migrations
uv run ruff check app tests migrations
uv run pytest -q
```

La suite cubre contratos, JSON inválido/NaN, persistencia, duplicados,
cuarentena, acústica, alertas, viajes, transición stale/offline, autenticación,
roles, REST, WebSocket, comandos y política de PUBACK. La migración inicial
también debe verificarse con `upgrade → downgrade → upgrade` antes de desplegar.

## Límites actuales

- La detección de viajes infiere movimiento por GPS; no afirma estado de
  encendido ni usa OBD.
- El backend conserva clasificación acústica relativa; no transforma dBFS en
  dB SPL ni afirma precisión real sin dataset etiquetado.
- La primera migración fue validada en SQLite para tests. La validación final
  contra PostgreSQL requiere un servidor o daemon Docker disponible.
- El frontend, simulador y contenedores de producción se construyen en fases
  separadas del repositorio.
