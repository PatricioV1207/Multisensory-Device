# VehicleSense — monitoreo vehicular IoT

VehicleSense es una plataforma general de monitoreo multisensorial para
vehículos. Integra firmware ESP32 tolerante a fallos, monitoreo local sin
Internet, entrega segura por HiveMQ Cloud, backend histórico, PostgreSQL,
aplicación web responsive y un simulador multivehículo.

```text
Sensores + ESP32 ── WiFi/MQTT TLS ──► HiveMQ Cloud
       │                                      │
       ├── página local                       ▼
       ├── JSONL + spool SD             FastAPI backend
       └── OTA local                          │
                                              ▼
                                         PostgreSQL
                                              │
                                    REST + WebSocket seguro
                                              │
                                              ▼
                                     React VehicleSense
```

El sistema no inventa ceros para datos ausentes: conserva banderas de validez,
omite `NaN` y distingue muestras reales, simuladas, en vivo y reproducidas.
SIM800L permanece en el repositorio como fase experimental diferida; la ruta de
producción actual usa WiFi.

## Componentes del repositorio

| Ruta | Responsabilidad |
|---|---|
| `src/`, `include/` | Firmware PlatformIO/Arduino para ESP32 |
| `contracts/` | Tópicos y JSON Schema compartidos |
| `backend/` | FastAPI, MQTT, PostgreSQL, REST, WebSocket y alertas |
| `frontend/` | React/TypeScript, dashboard, mapas e históricos |
| `simulator/` | Dispositivos MQTT sintéticos identificados como demo |
| `deploy/` | Docker Compose, Nginx, HTTPS, backup y guía OCI |
| `docs/` | Cableado, pruebas, telemetría, seguridad y operación |

## Hardware y pinout actual

- ESP32 DevKit V1 / NodeMCU-32 de 30 pines.
- DHT11 en GPIO27.
- GPS NEO-6M por UART2, GPIO32/33.
- GY-801 y BH1750 por I2C, GPIO21/22.
- microSD por SPI: GPIO18/19/23 y CS GPIO5.
- INMP441 por I2S: BCLK GPIO26, WS GPIO25 y SD GPIO34.
- SIM800L conservado por UART1 GPIO16/17, con fuente independiente; no forma
  parte de la ruta WiFi activa.

El GY-801 usa controladores independientes para ADXL345, L3G4200D, HMC5883L y
BMP180. Las calibraciones existentes del ADXL345 y BMP180/NVS se conservan. El
detalle eléctrico y los riesgos están en [docs/wiring.md](docs/wiring.md).

## Firmware

`src/main.cpp` delega en `AppController`; sensores, validación, serialización,
almacenamiento, web y transporte permanecen separados. El environment principal
actual es `vehiclesense_wifi`:

- AP+STA y monitor local sin dependencias externas;
- NTP con fallback de tiempo GPS;
- MQTT 3.1.1/TLS 8883, hostname validado, QoS 1, LWT y tópicos por identidad;
- JSONL de auditoría y spool microSD acotado hasta PUBACK;
- DHT11, GPS, GY-801, BH1750 e INMP441;
- resumen acústico dBFS relativo, clasificador heurístico conservador y modo de
  recolección de características sin audio crudo;
- OTA local protegida, con primera carga y recuperación por USB.

Configuración inicial:

```bash
cp include/secrets_example.h include/secrets.h
pio run -e test_i2c_scanner -t upload
pio device monitor -b 115200
pio run -e vehiclesense_wifi
```

`include/secrets.h` está ignorado por Git. No coloque credenciales reales en
`config.h`, `platformio.ini` o documentación. Consulte
[docs/architecture.md](docs/architecture.md),
[docs/local_web_ota.md](docs/local_web_ota.md) y
[docs/acoustic_monitoring.md](docs/acoustic_monitoring.md).

## HiveMQ Cloud y contratos

El prefijo es `vehiclesense/v1` y cada rama incluye `vehicle_id/device_id`.
Telemetría usa schema v3; status, acústica, eventos, comandos y acuses tienen
contratos propios. El firmware no se conecta hasta disponer de una hora UTC
confiable para validar certificados.

La configuración exacta de cluster, credenciales y ACL está en
[docs/hivemq_cloud.md](docs/hivemq_cloud.md). Los tópicos y payloads viven en
[contracts/mqtt-topics.md](contracts/mqtt-topics.md) y
[docs/telemetry_payload.md](docs/telemetry_payload.md).

## Backend y frontend

Desarrollo local del backend:

```bash
cd backend
cp .env.example .env
uv sync --extra dev
uv run alembic upgrade head
uv run uvicorn app.main:app --reload
```

Desarrollo del frontend:

```bash
cd frontend
cp .env.example .env
npm ci
npm run dev
```

El navegador obtiene histórico mediante REST y cambios mediante un WebSocket
FastAPI autenticado. Nunca recibe credenciales HiveMQ o PostgreSQL. API, roles,
modelo de datos y eventos se documentan en [backend/README.md](backend/README.md);
las pantallas, estados y demo explícita en [frontend/README.md](frontend/README.md).

## Desarrollo sin hardware

El frontend incluye un demo visual explícito. Para probar la cadena completa,
el simulador publica datos válidos por HiveMQ y los marca con `simulated=true`:

```bash
cd simulator
uv sync --extra dev
uv run vehiclesense-simulator \
  --dry-run --cycles 5 --vehicles 3 --scenario mixed
```

La guía de credenciales, escenarios, duplicados y pruebas negativas está en
[simulator/README.md](simulator/README.md).

## Producción

`deploy/compose.production.yml` ejecuta PostgreSQL, FastAPI, frontend estático y
Nginx. Solo Nginx publica 80/443. La guía de
[despliegue Oracle Cloud](deploy/README.md) cubre Docker en ARM64/amd64, DNS,
Certbot, health checks, registro de dispositivos, backups, restauración,
actualización y rollback.

## Validación

Firmware nativo:

```bash
pio test -e test_payload_json
pio test -e test_barometer_math
pio test -e test_local_web_json
pio test -e test_mqtt_topics
pio test -e test_offline_queue
pio test -e test_acoustic_classifier
```

Software cloud:

```bash
(cd backend && uv run ruff check . && uv run pytest -q)
(cd frontend && npm run lint && npm test -- --run && npm run build)
(cd simulator && uv run ruff check . && uv run pytest -q)
docker compose --env-file deploy/.env.example \
  -f deploy/compose.production.yml config --quiet
```

El procedimiento físico environment por environment y los criterios PASS/FAIL
están en [docs/tests.md](docs/tests.md).

## Estado y límites honestos

Implementado:

- firmware modular, monitor local, OTA, microSD y replay offline;
- telemetría VehicleSense v3 y MQTT seguro HiveMQ;
- INMP441, características acústicas y recolección etiquetada;
- backend FastAPI/PostgreSQL, alertas, viajes, roles, REST y WebSocket;
- frontend responsive basado en las dos referencias visuales;
- simulador multivehículo y despliegue OCI contenerizado.

Requiere acciones externas antes de declarar producción:

- probar físicamente INMP441 y recopilar un dataset real balanceado;
- crear credenciales/ACL HiveMQ y probar ESP32 ↔ broker ↔ backend;
- ejecutar migraciones y pruebas sobre PostgreSQL real;
- construir y levantar los contenedores con un daemon Docker disponible;
- aprovisionar VM, dominio, DNS, certificados y copias externas;
- realizar una prueba integral prolongada y de pérdida de conectividad.

Los niveles acústicos son dBFS relativos, no dB SPL. El clasificador es una
heurística experimental, no un detector validado. Los viajes se infieren por
GPS y no afirman estado de encendido. OTA local aún no firma criptográficamente
el firmware. Consulte [docs/security.md](docs/security.md) para el modelo de
confianza y las limitaciones restantes.
