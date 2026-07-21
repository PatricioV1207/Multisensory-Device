# Payload de telemetría

## Política temporal e identidad

`uptime_ms` es tiempo desde el arranque, no Unix epoch. El perfil VehicleSense
usa `schema_version=3`, `vehicle_id`, `device_id`, `boot_id`, `sequence`,
`sample_id` e identidad persistente. Cuando NTP o GPS proporcionan UTC
confiable incluye `measured_at` y `time_valid=true`; si no, omite la fecha en
vez de inventarla. El backend guarda además su `received_at`. El esquema v2 se
mantiene para los perfiles heredados y ThingsBoard.

## Campos

| Grupo | Campos | Unidad |
|---|---|---|
| Identidad v3 | `schema_version`, `vehicle_id`, `device_id`, `boot_id`, `sequence`, `sample_id`, `uptime_ms`, `measured_at` | — |
| DHT11 | `temperature_c`, `humidity_percent` | °C, % |
| GPS | `latitude`, `longitude`, `altitude_m`, `speed_kmh`, `satellites` | grados, m, km/h |
| Acelerómetro | `accel_x`, `accel_y`, `accel_z` | m/s² calibrados |
| Giroscopio | `gyro_x`, `gyro_y`, `gyro_z` | rad/s |
| Magnetómetro | `mag_x`, `mag_y`, `mag_z` | µT |
| Barómetro | `pressure_hpa`, `sea_level_pressure_hpa`, `baro_temperature_c`, `baro_altitude_m` | hPa, °C, m |
| Luz | `light_lux` | lx |
| Estado | `sd_valid`, `wifi_connected`, `wifi_rssi_dbm`, `mqtt_connected`, contadores `offline_*` | — |
| Celular heredado | `sim_valid`, `gprs_connected`, `gsm_csq` | — |
| Acústica v3 | `mic_valid`, `acoustic_valid`, `acoustic_relative_level_dbfs`, `acoustic_peak_dbfs`, `acoustic_category`, `acoustic_confidence`, `acoustic_clipping` | dBFS relativo, — |

Las banderas `dht_valid`, `gps_valid`, `accel_valid`, `gyro_valid`,
`mag_valid`, `baro_valid` e `imu_valid` siempre aparecen. `imu_valid` exige
acelerómetro, giroscopio y magnetómetro válidos; no depende del barómetro.
En v3 también aparecen siempre `bh1750_valid`, `sd_valid`, `wifi_connected`,
`mqtt_connected`, `mic_valid` y `acoustic_valid`. Los campos `sim_valid`,
`gprs_connected` y `gsm_csq` pertenecen al perfil celular heredado;
`gsm_csq` se omite cuando el módem devuelve señal desconocida.

Los campos `accel_x`, `accel_y` y `accel_z` corresponden a la lectura calibrada
del ADXL345. La lectura raw se conserva internamente para diagnóstico y aparece
en `test_adxl345`, pero no se publica en el payload MQTT.

`pressure_hpa` es la presión local corregida por
`BARO_PRESSURE_OFFSET_HPA`. `sea_level_pressure_hpa` es la referencia
normalizada que se utiliza para calcular `baro_altitude_m`; puede proceder del
fallback de configuración o de una calibración guardada en NVS. La presión raw
del BMP180 se conserva para `test_bmp180`, pero no se publica por MQTT.

La presión local y la presión a nivel del mar no son intercambiables. Una
aplicación meteorológica suele mostrar presión reducida al nivel del mar y no
debe utilizarse directamente para calcular el offset físico del sensor.

## Fallos parciales

Un dato inválido se omite. Por ejemplo, si el GPS aún no tiene fix:

```json
{
  "schema_version": 3,
  "message_type": "telemetry",
  "vehicle_id": "vehicle-001",
  "device_id": "device-001",
  "boot_id": 1234,
  "sequence": 12,
  "sample_id": "device-001:1234:12",
  "uptime_ms": 12000,
  "time_valid": false,
  "replayed": false,
  "simulated": false,
  "dht_valid": true,
  "gps_valid": false,
  "accel_valid": false,
  "gyro_valid": false,
  "mag_valid": false,
  "baro_valid": false,
  "imu_valid": false,
  "bh1750_valid": false,
  "sd_valid": true,
  "wifi_connected": true,
  "wifi_rssi_dbm": -58,
  "mqtt_connected": true,
  "mic_valid": false,
  "acoustic_valid": false,
  "sim_valid": false,
  "gprs_connected": false,
  "temperature_c": 25.0,
  "humidity_percent": 60.0
}
```

No se serializan `NaN`, `null` ni valores anteriores obsoletos.

## Mensajes acústicos

El tópico `acoustic` usa un contrato separado `acoustic-v1` con agregados de
aproximadamente un segundo, características espectrales y proporciones de
energía por banda. Los eventos sostenidos se publican como `event-v1`. El
payload de telemetría v3 conserva solo el resumen más reciente.

`relative_level_dbfs` y `peak_dbfs` son relativos al máximo digital. No son
mediciones dB SPL calibradas. Las categorías provienen de una heurística
versionada y deben interpretarse como candidatas experimentales, no como una
identificación acústica certificada. El dispositivo no transmite ni almacena
audio crudo.
