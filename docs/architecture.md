# Arquitectura del firmware

## Flujo de datos

```text
DHT11 ─┐
GPS ───┼─> datos tipados ─> TelemetryValidator ─> TelemetryBuilder ─> MQTT
GY-801 ┘                               │                  │
                                      └──────────────> Serial
```

`AppController` es el único orquestador. Los sensores no conocen la red ni el
formato JSON, y los clientes de comunicación no conocen registros I2C.

## Componentes

- `DHT11Sensor`: temperatura y humedad con lectura cada 2 segundos.
- `GPSNeo6M`: vacía UART2 en cada iteración para no perder sentencias NMEA.
- `GY801IMU`: fachada de acelerómetro, giroscopio, magnetómetro y barómetro.
- `TelemetryValidator`: invalida datos fuera de rango, no finitos u obsoletos.
- `TelemetryBuilder`: genera el esquema v1 y omite mediciones inválidas.
- `WiFiManagerCustom`: conexión y backoff sin esperar en bucles bloqueantes.
- `MQTTClientCustom`: MQTT 3.1.1 sobre cualquier implementación de `Client`.
- `ModuleTestRunner`: inicializa solo el bloque seleccionado por `APP_MODE`.

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

