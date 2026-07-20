# VehicleSense: auditoría técnica y plan de implementación

Estado: fase 0 completada (auditoría y diseño; sin implementación funcional)

Fecha de auditoría: 2026-07-20

## 1. Objetivo y límites de esta fase

VehicleSense evolucionará el prototipo ESP32 existente hacia una plataforma de monitoreo para vehículos de propósito general. La solución final comprenderá:

- firmware ESP32 con sensores, GPS, microSD, página local, OTA y telemetría segura por WiFi;
- adquisición acústica relativa mediante INMP441;
- HiveMQ Cloud como broker MQTT externo;
- backend FastAPI como único consumidor MQTT de confianza;
- PostgreSQL para estado, historial, alertas y viajes;
- frontend web responsive para flota y detalle de vehículo;
- simulador multivehículo;
- despliegue reproducible en Oracle Cloud con Docker Compose y Nginx.

Esta fase solo establece el estado real del repositorio, las decisiones de arquitectura, el orden seguro de implementación y los criterios de aceptación. No modifica firmware, pines, credenciales ni environments.

Restricciones preservadas:

- mantener las calibraciones existentes del ADXL345 y BMP180;
- mantener todos los environments y diagnósticos actuales;
- usar WiFi como transporte activo en esta etapa;
- conservar SIM800L para una fase posterior, sin depurarlo ni eliminarlo;
- no publicar números falsos, `NaN`, fechas inventadas ni estados no observados;
- no afirmar medición real de dB SPL ni clasificación acústica confiable sin calibración y datos etiquetados;
- no exponer credenciales MQTT al navegador.

## 2. Resultado de la auditoría

### 2.1 Estado del repositorio

- Rama auditada: `main`.
- Commit base: `84caa65` (`Implementation Storage and Communication`).
- Worktree inicial: limpio.
- Proyecto actual: firmware PlatformIO/Arduino para ESP32; todavía no existen backend, frontend cloud, simulador ni infraestructura.
- Tamaño aproximado auditado: 5156 líneas entre código, pruebas y documentación.
- PlatformIO: `espressif32` 7.0.1.
- Arduino ESP32: 2.0.17, basado en ESP-IDF 4.4.x.

### 2.2 Baseline reproducible

Compilaron correctamente los 21 environments ESP32 existentes:

```text
full_prototype
test_dht11
test_gps
test_i2c_scanner
test_adxl345
test_l3g4200d
test_hmc5883l
test_bmp180
calibrate_bmp180
test_gy801
test_wifi
test_mqtt_wifi
test_bh1750
test_microsd
test_sim800l_at
test_sim800l_gprs
test_sim800l_mqtt
test_sim800l_mqtt_tls
test_local_web
test_local_ota
full_prototype_cellular
```

También pasaron 13 de 13 pruebas nativas:

- `test_payload_json`: 6 pruebas;
- `test_barometer_math`: 4 pruebas;
- `test_local_web_json`: 3 pruebas.

Consumo observado en builds de referencia:

| Environment | RAM | Flash |
|---|---:|---:|
| `full_prototype` | 61 068 B (18.6 %) | 861 953 B (65.8 %) |
| `full_prototype_cellular` | 42 016 B (12.8 %) | 542 473 B (27.6 % del slot OTA) |

La diferencia de tamaños se debe a selección y enlace condicional por modo; no significa que el perfil celular contenga más funcionalidad activa en la imagen final.

### 2.3 Lo que ya funciona y se conservará

- `main.cpp` delega en `AppController`.
- Sensores desacoplados con `begin()`, `update()`, `isValid()` y `getData()`.
- DHT11, NEO-6M, GY-801 y BH1750 con validez independiente.
- ADXL345 con calibración de seis posiciones y resultados raw/calibrados.
- BMP180 con referencia barométrica persistente en NVS.
- GPS con distinción entre ausencia de bytes, NMEA sin fix y fix fresco.
- JSON que omite valores inválidos y conserva banderas de validez.
- microSD con JSON Lines, rotación y contador de sesión.
- página local y OTA embebidas, sin archivos web externos.
- reconexión no bloqueante de WiFi/MQTT y continuidad de sensores ante fallos.
- soporte SIM800L/TinyGSM aislado y preservable.

### 2.4 Brechas encontradas

El perfil WiFi actual no es todavía el producto VehicleSense completo:

- MQTT por WiFi usa TCP sin TLS.
- PubSubClient publica únicamente con QoS 0 y no cubre LWT/estado retenido como requiere el diseño final.
- `full_prototype` no inicia BH1750, microSD ni página local.
- la página local existe solo en el perfil celular y muestra SIM/GPRS, no el estado WiFi objetivo;
- la página local actual omite GPS completamente; el objetivo requiere estado y coordenadas en texto, nunca mapa local;
- no existe `vehicle_id`, secuencia estable, timestamp de medición válido, marca de replay ni esquema acústico;
- la microSD archiva, pero no implementa una cola offline acotada con confirmación y reenvío;
- no existen tópicos separados de telemetría, estado, eventos, acústica, comandos y acuses;
- no existen INMP441, backend, base de datos, frontend cloud, simulador ni despliegue;
- todavía queda terminología específica de buses y ThingsBoard en configuración/documentación;
- `docs/pruebas_realizadas.md` registra un aviso `Bus already started` de Wire que debe reproducirse y resolverse sin alterar calibraciones.

## 3. Decisiones cerradas de arquitectura

### 3.1 Estructura del repositorio

El firmware permanecerá en la raíz para evitar una migración disruptiva. Se agregarán componentes hermanos:

```text
Proyecto_Monitoreo_Automovil/
├── include/                 # configuración y contratos firmware
├── src/                     # firmware ESP32 existente
├── test/                    # pruebas nativas y de hardware
├── docs/                    # arquitectura, operación y contratos
├── backend/                 # FastAPI, MQTT, PostgreSQL y migraciones
├── frontend/                # React + TypeScript + Vite
├── simulator/               # productor MQTT multivehículo
├── deploy/                  # Compose, Nginx, backup y operación OCI
├── platformio.ini
└── README.md
```

No se moverán archivos existentes solo por estética. Cada migración deberá mantener compilación y compatibilidad.

### 3.2 Perfiles de firmware

Se añadirá `vehiclesense_wifi` como perfil objetivo. `full_prototype` se mantendrá como alias de compatibilidad hasta que una migración documentada permita unificar ambos.

Nuevos environments previstos:

- `test_inmp441`: adquisición I2S y diagnóstico raw normalizado;
- `collect_acoustic_features`: recolección etiquetada de características;
- `test_acoustic_classifier`: clasificación y alertas con fixtures;
- `test_mqtt_hivemq_tls`: TLS, ACL, LWT, QoS y reconexión;
- `test_offline_queue`: cola, replay, límites y recuperación;
- `vehiclesense_wifi`: integración completa de la fase WiFi.

Ningún environment actual será eliminado ni renombrado.

### 3.3 Pines del INMP441

Propuesta sin conflictos con el mapa actual:

| INMP441 | ESP32 | Motivo |
|---|---:|---|
| SCK/BCLK | GPIO26 | salida I2S libre |
| WS/LRCLK | GPIO25 | salida I2S libre |
| SD | GPIO34 | entrada solamente, apropiada para datos del micrófono |
| L/R | GND | canal izquierdo |
| VDD | 3V3 | lógica y alimentación a 3.3 V |
| GND | GND | tierra común |

Se preservan DHT11 GPIO27, I2C 21/22, microSD 18/19/23/5, GPS 32/33 y SIM800L 16/17. Antes de conectar físicamente se deberá verificar que GPIO25, GPIO26 y GPIO34 estén expuestos y correctamente rotulados en la variante exacta de DevKit de 30 pines.

El core local incluye la biblioteca Arduino `I2S` y el driver IDF `driver/i2s.h`. Para minimizar incertidumbre con Arduino 2.0.17 se comenzará con la API disponible en ese core, no con ejemplos escritos exclusivamente para Arduino ESP32 3.x.

### 3.4 Arquitectura acústica

El módulo se separará en responsabilidades comprobables:

```text
INMP441Microphone
  └─ AudioFeatureExtractor
       ├─ AcousticClassifier
       ├─ AcousticAlertEvaluator
       └─ AcousticDatasetLogger
```

Configuración inicial propuesta:

- muestreo: 16 kHz;
- trama I2S: 32 bits, extrayendo la muestra válida de 24 bits;
- ventana: 1024 muestras (64 ms);
- salto: 512 muestras;
- agregación publicada: 1 segundo;
- eliminación de DC y filtro pasaaltos aproximado de 80–100 Hz;
- ventana Hann para FFT;
- procesamiento en tarea FreeRTOS acotada, con snapshot o cola pequeña para no bloquear GPS, MQTT ni watchdog.

Características v1:

- RMS y pico relativos en dBFS;
- relación de clipping;
- factor de cresta;
- tasa de cruces por cero;
- centroide, planitud y rolloff espectral;
- energía relativa en bandas 80–250, 250–800, 800–2000, 2000–4000 y 4000–8000 Hz.

Categorías heurísticas:

```text
quiet, wind, engine, speech, music, horn, siren, traffic, unknown
```

Reglas de honestidad:

- `relative_level_dbfs` no se etiquetará como dB SPL;
- `unknown` será la salida por defecto cuando no haya evidencia suficiente;
- toda clasificación incluirá `classifier_version` y `confidence`;
- clipping, micrófono inválido o confianza baja impiden generar una alerta acústica;
- no se realizará reconocimiento de voz ni se almacenará conversación;
- el guardado de audio raw estará deshabilitado por defecto y requerirá un build explícito, aviso de privacidad y espacio controlado.

El primer clasificador será heurístico y se considerará experimental. Una mejora a modelo estadístico requerirá grabaciones etiquetadas representativas del vehículo y del entorno.

### 3.5 Transporte MQTT seguro

El core instalado ya incluye ESP-MQTT con TLS, QoS 0/1/2, LWT, reconexión, suscripciones y cola asíncrona. Se reemplazará el interior de la capa MQTT WiFi, conservando una fachada estable para `AppController`. Esto evita reescribir sensores, telemetría o SIM800L.

Tópicos versionados:

```text
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/telemetry
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/status
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/events
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/acoustic
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/commands
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/command-acks
```

Política inicial:

| Flujo | QoS | Retain |
|---|---:|---:|
| telemetría | 1 | no |
| acústica agregada | 1 | no |
| eventos/alertas | 1 | no |
| estado online | 1 | sí |
| LWT offline | 1 | sí |
| comandos | 1 | no |
| acuses | 1 | no |

La conexión será MQTT/TLS en 8883 con validación de CA y hostname. No se aceptará `setInsecure()` en el perfil de producción.

Credenciales y ACL:

- una credencial distinta por dispositivo o un patrón seguro permitido por el plan de HiveMQ elegido;
- el dispositivo publica solo en sus tópicos y se suscribe solo a `commands` propio;
- el backend utiliza otra credencial con lectura de dispositivos y publicación controlada de comandos;
- el frontend nunca conoce credenciales MQTT;
- secretos reales permanecen fuera de Git;
- no se concederá `#` a dispositivos.

### 3.6 Identidad, tiempo y esquema de telemetría

Se conservará `device_id` por compatibilidad y se añadirá `vehicle_id`. La relación vehículo–dispositivo será explícita en la base de datos para permitir reemplazar hardware sin perder el historial del vehículo.

Esquema firmware propuesto: versión 3.

Metadatos mínimos:

```json
{
  "schema_version": 3,
  "device_id": "esp32-001",
  "vehicle_id": "vehicle-001",
  "boot_id": 123,
  "sequence": 4567,
  "sample_id": "esp32-001:123:4567",
  "uptime_ms": 987654,
  "time_valid": true,
  "measured_at": "2026-07-20T23:18:51Z",
  "replayed": false,
  "simulated": false
}
```

Reglas:

- `measured_at` solo existe cuando NTP o fecha/hora GPS es válida;
- el backend siempre agrega `received_at`;
- `sample_id` es estable durante replay e idempotente en base de datos;
- los nombres actuales de sensores se conservan durante la migración;
- valores inválidos se omiten, con banderas de validez presentes;
- los cambios incompatibles exigen subir `schema_version` y mantener un adaptador temporal en backend.

### 3.7 Almacenamiento y operación offline

Se separarán dos conceptos:

1. **Archivo local:** JSONL rotativo con todas las muestras para auditoría.
2. **Spool de entrega:** cola acotada de muestras no confirmadas por MQTT QoS 1.

El spool tendrá límites configurables de bytes, archivos y antigüedad. Al recuperar conexión:

- reenvía desde el elemento más antiguo;
- conserva `sample_id` y tiempo original;
- marca `replayed=true`;
- limita la velocidad para no bloquear telemetría en vivo;
- elimina solo después de confirmación PUBACK;
- cuenta `queued`, `replayed`, `dropped` y `oldest_age_s`;
- ante llenado aplica una política explícita de descarte de lo más antiguo y genera evento de pérdida.

Si no hay SD, podrá existir un ring buffer RAM pequeño y fijo. Nunca se usará una cola RAM o SD sin límite.

### 3.8 Página local ESP32

Se reutilizarán `LocalWebServer`, `LocalWebJsonBuilder` y los assets PROGMEM actuales. El objetivo no es crear una segunda aplicación web pesada.

El perfil WiFi deberá operar en `WIFI_AP_STA`:

- STA para Internet/HiveMQ;
- AP WPA2 para diagnóstico local;
- sensores y SD continúan si una de las redes falla.

La página mostrará:

- `device_id`, `vehicle_id`, firmware y uptime;
- estado WiFi, RSSI, IP STA e IP AP;
- MQTT y tiempo/resultado del último envío;
- salud de cada sensor;
- temperatura, humedad, presión, luz y resumen IMU;
- GPS válido/no válido y coordenadas en texto, sin mapa;
- microSD, cola offline y contadores;
- INMP441, nivel relativo, categoría, confianza y clipping;
- alertas activas recientes.

Se conservarán `/`, `/api/status`, `/api/telemetry/basic`, `/admin` y `/admin/update`, versionando respuestas sin romper consumidores. HTML, CSS y JavaScript seguirán embebidos y sin CDN. OTA continuará protegida por WPA2 + HTTP Basic y deberá pausar escrituras/publicaciones solo durante la transferencia.

### 3.9 Backend y API

Stack propuesto:

- Python 3.12;
- FastAPI;
- SQLAlchemy 2 async + asyncpg;
- Alembic;
- Pydantic 2;
- cliente MQTT asíncrono con TLS;
- PostgreSQL 16;
- JWT con access/refresh y contraseñas Argon2;
- roles `admin`, `operator`, `viewer`.

Procesos principales:

```text
HiveMQ Cloud
  → consumidor MQTT con reconexión
  → validación por schema_version
  → cola interna acotada
  → transacción PostgreSQL idempotente
  → evaluadores de estado/alertas/viajes
  → REST histórico + WebSocket en vivo
```

API inicial:

```text
POST /api/v1/auth/login
POST /api/v1/auth/refresh
POST /api/v1/auth/logout
GET  /api/v1/vehicles
GET  /api/v1/vehicles/{id}
GET  /api/v1/vehicles/{id}/latest
GET  /api/v1/vehicles/{id}/telemetry
GET  /api/v1/vehicles/{id}/route
GET  /api/v1/vehicles/{id}/trips
GET  /api/v1/alerts
POST /api/v1/alerts/{id}/acknowledge
POST /api/v1/commands
GET  /api/v1/devices/{id}/status-history
GET  /api/v1/health/live
GET  /api/v1/health/ready
WS   /api/v1/ws
```

La API debe paginar históricos, limitar rangos, validar permisos por vehículo y no retornar cero cuando falta un dato.

### 3.10 Modelo de datos

Tablas mínimas:

- `users` y sesiones/tokens de refresh;
- `vehicles`;
- `devices` y asignación temporal a vehículo;
- `telemetry_samples`;
- `acoustic_measurements`;
- `acoustic_events`;
- `alerts` y `alert_rules`;
- `trips`;
- `device_status_history`;
- `commands` y `command_acks`.

Restricciones importantes:

- unicidad de `(device_id, sample_id)` para tolerar replay;
- índices por vehículo y tiempo descendente;
- `received_at` obligatorio, `measured_at` opcional;
- latitud/longitud opcionales, nunca `0,0` por ausencia de fix;
- JSON original opcional para auditoría, con límite de tamaño;
- migraciones Alembic reversibles o con procedimiento de respaldo.

PostGIS no será requisito inicial: `double precision` y distancia Haversine son suficientes para el prototipo. Se documentará una migración futura si aparecen consultas geoespaciales complejas.

### 3.11 Alertas y viajes

Estados de alerta:

```text
active → acknowledged → resolved
```

Cada regla tendrá severidad, umbral, duración mínima, cooldown y overrides por vehículo. Se deduplicará por `vehicle_id + rule_type + ventana/estado`.

Reglas iniciales candidatas:

- dispositivo offline o stale;
- temperatura alta;
- GPS inválido prolongado;
- SD inválida o spool con pérdidas;
- clipping/micrófono inválido;
- evento acústico sostenido de tráfico, bocina o sirena con confianza suficiente;
- aceleración anómala solo después de validar orientación, montaje y umbrales físicos.

No se implementarán afirmaciones como “frenado brusco” hasta validar ejes, tasa de muestreo y umbral con datos reales.

Detección de viajes sin señal de ignición:

- inicio candidato: GPS válido y velocidad mayor a 3 km/h sostenida 60 s;
- cierre candidato: velocidad menor a 2 km/h sostenida 5 min;
- umbrales configurables;
- rechazo de fixes con HDOP/antigüedad fuera de rango;
- distancia Haversine con filtro de saltos;
- estado documentado como inferido, no como encendido/apagado real.

### 3.12 Frontend cloud

Stack propuesto:

- React + TypeScript + Vite;
- React Router;
- TanStack Query para REST;
- WebSocket nativo para datos recientes;
- Leaflet + OpenStreetMap;
- Recharts para series temporales;
- Vitest/Testing Library y Playwright.

Las dos referencias visuales suministradas definen:

1. tablero general con navegación lateral, KPIs, mapa de flota, vehículos, alertas, viajes y tendencias;
2. detalle de vehículo con estado, ruta, métricas, conectividad, telemetría y eventos.

Se conservarán sus principios visuales —fondo claro, azul/cian/teal, tarjetas, jerarquía compacta y estados por color— sin copiar datos ficticios. En particular:

- no se mostrará 4G mientras el transporte activo sea WiFi;
- “fuel level”, conductor, matrícula, marca/modelo y foto solo aparecerán si el backend contiene esos datos;
- el ruido se mostrará como nivel relativo, nunca como dB SPL;
- el mapa y las rutas solo aparecen con GPS válido;
- las cifras demo se identificarán claramente como simuladas.

Estados de interfaz obligatorios:

- loading y vacío;
- vehículo offline o stale;
- sin GPS;
- backend no disponible;
- WebSocket reconectando;
- permiso insuficiente;
- historial parcial;
- dispositivo sin un sensor opcional.

### 3.13 Simulador

El simulador Python producirá múltiples vehículos por MQTT con credenciales y prefijo claramente separados de producción. Generará:

- rutas GPS plausibles;
- temperatura, humedad, presión, luz e IMU;
- estados online/offline/stale;
- categorías acústicas y eventos;
- desconexiones, replay y secuencias duplicadas;
- alertas controladas para pruebas de UI.

Todo mensaje simulado incluirá `simulated=true`. Nunca se activará como fallback silencioso cuando falten datos reales.

### 3.14 Despliegue Oracle Cloud

Topología de producción:

```text
Internet
  → Nginx :443
       ├─ frontend estático
       ├─ /api → FastAPI
       └─ /ws  → FastAPI WebSocket
  → PostgreSQL en red Docker privada

ESP32/simulador/backend MQTT → HiveMQ Cloud externo
```

Artefactos previstos:

- Dockerfile multi-stage del backend;
- Dockerfile multi-stage del frontend;
- `compose.production.yml` compatible con ARM64;
- Nginx con HTTPS, headers, compresión, límites y proxy WebSocket;
- volúmenes persistentes;
- health checks y `restart: unless-stopped`;
- scripts de `pg_dump`, restore, actualización y rollback;
- `.env.example` sin secretos.

Solo Nginx expondrá 80/443; PostgreSQL no tendrá puerto público. SSH usará llave y reglas OCI/UFW restringidas. La capacidad Always Free de Oracle puede cambiar o no estar disponible en una región, por lo que el sizing se confirmará antes de crear la instancia.

## 4. Plan de implementación por fases

### Fase 0 — Auditoría y contratos

Entregables:

- este documento;
- baseline de compilación y pruebas;
- mapa de pines propuesto;
- decisiones de esquema, tópicos y stack.

Criterio de salida: repositorio limpio salvo el documento y ninguna regresión funcional.

### Fase 1 — Terminología, contratos y esqueleto

Trabajo:

- introducir VehicleSense, `vehicle_id` y compatibilidad con `device_id` existente;
- especificar JSON Schema v3 y tópicos MQTT;
- crear directorios backend/frontend/simulator/deploy con README y contratos compartidos;
- documentar migración desde schema v2;
- añadir fixtures válidos, parciales, obsoletos y malformados.

Pruebas:

- todos los environments actuales;
- JSON Schema y compatibilidad v2/v3;
- comprobación de que no se filtran secretos.

### Fase 2 — Firmware WiFi seguro y resiliencia

Trabajo:

- perfil `vehiclesense_wifi`;
- AP+STA;
- NTP con fallback de fecha/hora GPS;
- ESP-MQTT TLS, LWT, QoS 1, estado y comandos;
- BH1750, microSD y web local en el perfil WiFi;
- archivo local y spool acotado;
- telemetría v3 y contadores de entrega.

Pruebas:

- CA/hostname inválido, credenciales incorrectas y ACL denegada;
- caída y retorno de WiFi/broker;
- QoS 1 y replay sin duplicar en el consumidor;
- SD retirada/llena/reinsertada;
- OTA y calibraciones persistentes tras reinicio.

### Fase 3 — INMP441 y acústica honesta

Trabajo:

- driver I2S y environment `test_inmp441`;
- extracción de características y pruebas nativas;
- calibración relativa de silencio/operación;
- clasificador heurístico versionado;
- lógica de alertas sostenidas;
- modo de recolección etiquetada;
- integración en JSON, SD, web local y tópicos.

Pruebas:

- silencio, tono, ruido, clipping, desconexión y saturación de CPU;
- continuidad de GPS/MQTT mientras se procesa audio;
- clasificación `unknown` ante ambigüedad;
- privacidad: raw deshabilitado por defecto.

Pausa física requerida: conectar el INMP441 solo después de confirmar el pinout real del DevKit y validar Serial con `test_inmp441`.

### Fase 4 — Backend, PostgreSQL y simulador

Trabajo:

- FastAPI, configuración y autenticación;
- migraciones y repositorios de datos;
- consumidor MQTT resiliente;
- estado/stale/offline;
- REST y WebSocket;
- simulador multivehículo.

Pruebas:

- pytest unitario e integración con PostgreSQL;
- schema desconocido, JSON malformado, duplicado y fuera de orden;
- reconexión MQTT y presión de cola;
- auth, roles, paginación, WS y health/readiness.

### Fase 5 — Alertas, viajes y comandos

Trabajo:

- motor de reglas, deduplicación, duración y cooldown;
- ciclo active/acknowledged/resolved;
- inferencia de viajes;
- comandos y acuses auditables.

Pruebas:

- fronteras temporales y de umbral;
- GPS inválido y saltos imposibles;
- alertas duplicadas/reabiertas;
- comandos expirados, no soportados y sin acuse.

### Fase 6 — Frontend funcional y visual

Trabajo:

- autenticación y shell de navegación;
- dashboard de flota;
- mapa y listado de vehículos;
- detalle de vehículo;
- alertas, viajes, dispositivos, analítica y configuración;
- REST histórico + WebSocket en vivo;
- adaptación responsive basada en las referencias suministradas.

Pruebas:

- Vitest/Testing Library;
- Playwright desktop/móvil;
- estados vacíos, offline, stale, sin GPS, error REST/WS y permisos;
- verificación de que ningún dato inexistente se presenta como real.

### Fase 7 — Despliegue y operación

Trabajo:

- imágenes ARM64;
- Compose de producción;
- Nginx/HTTPS;
- provisionamiento OCI;
- backup/restore;
- actualización y rollback documentados.

Pruebas:

- smoke test externo;
- reinicio de contenedores y host;
- recuperación de base desde backup;
- renovación TLS;
- rollback a imagen anterior;
- puertos y secretos auditados.

### Fase 8 — End-to-end y endurecimiento

Trabajo:

- ejecución sostenida con ESP32 y simuladores;
- métricas operativas y logs estructurados;
- límites, retención y limpieza;
- documentación final y matriz de trazabilidad.

Criterio final:

- pérdida de red no detiene adquisición ni almacenamiento local;
- replay no duplica muestras;
- el frontend distingue real, simulado, stale y ausente;
- los secretos no están en firmware público, repositorio ni navegador;
- el INMP441 se describe y usa como medición relativa;
- todas las pruebas automatizadas y físicas definidas pasan.

## 5. Riesgos y decisiones que requieren evidencia

| Riesgo | Mitigación / puerta de avance |
|---|---|
| Pinout distinto del DevKit de 30 pines | verificación visual antes de cablear INMP441 |
| Carga I2S/FFT afecta GPS o watchdog | tarea acotada, medición de latencia y prueba prolongada |
| Clasificación acústica poco fiable | heurística explícita, `unknown`, confianza y dataset etiquetado |
| Confundir dBFS con dB SPL | nombres/unidades explícitos y sin conversión SPL no calibrada |
| PubSubClient no publica QoS 1 | adaptar WiFi a ESP-MQTT, preservando fachada |
| TLS consume flash/RAM | medir cada build y mantener partición OTA válida |
| ACL HiveMQ demasiado amplia | credencial por identidad y pruebas positivas/negativas |
| Cola offline llena la SD | límites, rotación, contadores y descarte explícito |
| GPS sin tiempo válido | NTP/GPS con `time_valid`; nunca inventar Unix time |
| Terminología bus rompe históricos | schema versionado y alias/migración compatible |
| Oracle Always Free no disponible | confirmar región/cupo y mantener Compose portable |
| Datos de UI no soportados | ocultar/estado vacío; simulator claramente rotulado |

## 6. Acciones manuales previstas

No se necesitan todavía para comenzar las fases puramente de software. Sí se solicitarán en el punto apropiado:

1. Confirmar físicamente GPIO25/26/34 y cablear INMP441.
2. Ejecutar `test_inmp441` y compartir una muestra Serial de silencio, voz, motor y tráfico.
3. Crear HiveMQ Cloud y entregar credenciales por archivo local no versionado.
4. Configurar ACL del dispositivo y del backend.
5. Aportar grabaciones/ventanas etiquetadas si se desea mejorar el clasificador.
6. Elegir región OCI, crear instancia y dominio cuando llegue la fase de despliegue.
7. Validar visualmente frontend desktop/móvil contra las dos referencias.

## 7. Política de cambios y commits

- un commit enfocado por hito estable;
- ejecutar baseline afectado antes de cada commit;
- no mezclar migraciones de firmware, backend y frontend sin necesidad;
- no sobrescribir cambios del usuario ni usar operaciones Git destructivas;
- documentar archivos, comandos, resultados y limitaciones al cerrar cada fase;
- detenerse antes de una migración destructiva, cambio de pines no verificado o eliminación de funcionalidad existente.

## 8. Fuentes técnicas principales

- [Arduino ESP32 I2S](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2s.html)
- [ESP-IDF MQTT](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/protocols/mqtt.html)
- [INMP441 datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/INMP441.pdf)
- [HiveMQ Cloud documentation](https://docs.hivemq.com/hivemq-cloud/)
- [Oracle Cloud Free Tier](https://docs.oracle.com/en-us/iaas/Content/FreeTier/freetier.htm)

