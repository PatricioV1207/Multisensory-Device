# Arquitectura del firmware

## Flujo de datos

```text
DHT11 ─┐
GPS ───┼─> datos tipados ─> TelemetryValidator ─> TelemetryBuilder ─> MQTT
GY-801 ┘                               │                  │
                                      └──────────────> Serial

BMP180 ─> BarometerMath <─ referencia config/NVS
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
- `TelemetryBuilder`: genera el esquema v1 y omite mediciones inválidas.
- `WiFiManagerCustom`: conexión y backoff sin esperar en bucles bloqueantes.
- `MQTTClientCustom`: MQTT 3.1.1 sobre cualquier implementación de `Client`.
- `ModuleTestRunner`: inicializa solo el bloque seleccionado por `APP_MODE`.

El BMP180 no conoce GPS ni NVS. `AppController` carga la referencia persistida
y la inyecta en la fachada GY-801. El environment `calibrate_bmp180` es el único
flujo que combina GPS y barómetro para calcular una nueva referencia.

## Temporización

El scheduler usa resta de `uint32_t` sobre `millis()`, por lo que tolera su
desbordamiento. GPS y mantenimiento MQTT se atienden en cada `loop()`; DHT,
GY-801 y telemetría usan intervalos independientes de `config.h`.

Los intentos MQTT son síncronos por limitación de PubSubClient, pero su socket
tiene timeout acotado. Las reconexiones usan backoff configurable entre 5 y 30
segundos.

## Extensiones futuras

Un nuevo sensor solo necesita producir datos y ser agregado a la instantánea.
La microSD podrá añadirse como otro consumidor del payload. Un módem celular
podrá entregar otro `Client` a `MQTTClientCustom` sin cambiar el builder.
