# Arquitectura del firmware

## Flujo de datos

```text
DHT11 ─┐
GPS ───┼─> datos tipados ─> TelemetryValidator ─> TelemetryBuilder v2/v3
GY-801 ┤                               │                  │
BH1750 ┘                               └──────────────> Serial
                                                          │
                          ┌──────── archivo JSONL <────────┤
                          └──────── spool QoS 1 ───────────┼─> HiveMQ/TLS
                                                         PUBACK

BMP180 ─> BarometerMath <─ referencia config/NVS
Estado local ─> WebServer AP+STA ─> dashboard + OTA
```

`AppController` es el único orquestador. Los sensores no conocen la red ni el
formato JSON, y los clientes de comunicación no conocen registros I2C.

## Componentes

- `DHT11Sensor`: temperatura y humedad con lectura cada 2 segundos.
- `GPSNeo6M`: vacía UART2 en cada iteración para no perder sentencias NMEA.
- `GY801IMU`: fachada de acelerómetro, giroscopio, magnetómetro y barómetro.
- `BarometerMath`: conversión pura entre presión local, presión a nivel del
  mar y altitud.
- `BarometerCalibrationStore`: carga y guarda en NVS la referencia confirmada.
- `BarometerCalibrationRunner`: diagnóstico de 60 segundos que calcula
  candidatos por altitud conocida y GPS, sin guardarlos automáticamente.
- `TelemetryValidator`: invalida datos fuera de rango, no finitos u obsoletos.
- `TelemetryBuilder`: conserva el esquema v2 y genera VehicleSense v3 con
  identidad persistente, hora confiable y estado de transporte.
- `WiFiManagerCustom`: conexión y backoff sin esperar en bucles bloqueantes.
- `MQTTClientCustom`: MQTT 3.1.1 sobre cualquier implementación de `Client`.
- `ModuleTestRunner`: inicializa solo el bloque seleccionado por `APP_MODE`.
- `BH1750Sensor`: lux por el mismo bus I2C del GY-801.
- `MicroSDLogger`: respaldo JSONL independiente de la conectividad.
- `OfflineTelemetryQueue`: spool SD acotado, checksum, replay ordenado y
  borrado únicamente tras PUBACK.
- `VehicleSenseMqttClient`: HiveMQ Cloud mediante TLS, QoS 1, LWT, ACL por
  tópico, backoff y correlación de PUBACK.
- `SIM800LModem`: estados AT, SIM, red 2G y GPRS.
- `LocalWebServer`: monitor local AP+STA con estado y coordenadas GPS en texto,
  sin mapa, además de administración OTA.

El BMP180 no conoce GPS ni NVS. `AppController` carga la referencia persistida
y la inyecta en la fachada GY-801. El environment `calibrate_bmp180` es el único
flujo que combina GPS y barómetro para calcular una nueva referencia.

## Temporización

El scheduler usa resta de `uint32_t` sobre `millis()`, por lo que tolera su
desbordamiento. GPS y mantenimiento MQTT se atienden en cada `loop()`; DHT,
GY-801 y telemetría usan intervalos independientes de `config.h`.

El perfil heredado usa PubSubClient. `vehiclesense_wifi` usa ESP-MQTT sobre TLS
y espera una hora confiable antes del handshake de certificados. Las
reconexiones usan backoff configurable entre 5 y 30 segundos. El spool limita
el replay y conserva muestras no confirmadas sin detener el `loop()`.

## Extensiones futuras

Un nuevo sensor solo necesita producir datos y ser agregado a la instantánea.
Otros módems pueden entregar un `Client` a `MQTTClientCustom` sin cambiar el
builder. El roadmap conserva fusión IMU, TLS con hardware moderno y backend
cloud con historial.
