# Payload de telemetría

## Política temporal

ThingsBoard asigna la fecha al recibir el mensaje. `uptime_ms` es tiempo desde
el arranque, no Unix epoch. Un broker genérico debe fechar el mensaje al
recibirlo o añadir NTP en una fase posterior.

## Campos

| Grupo | Campos | Unidad |
|---|---|---|
| Identidad | `schema_version=2`, `device_id`, `uptime_ms` | — |
| DHT11 | `temperature_c`, `humidity_percent` | °C, % |
| GPS | `latitude`, `longitude`, `altitude_m`, `speed_kmh`, `satellites` | grados, m, km/h |
| Acelerómetro | `accel_x`, `accel_y`, `accel_z` | m/s² calibrados |
| Giroscopio | `gyro_x`, `gyro_y`, `gyro_z` | rad/s |
| Magnetómetro | `mag_x`, `mag_y`, `mag_z` | µT |
| Barómetro | `pressure_hpa`, `sea_level_pressure_hpa`, `baro_temperature_c`, `baro_altitude_m` | hPa, °C, m |
| Luz | `light_lux` | lx |
| Estado | `sd_valid`, `sim_valid`, `gprs_connected`, `gsm_csq` | — |

Las banderas `dht_valid`, `gps_valid`, `accel_valid`, `gyro_valid`,
`mag_valid`, `baro_valid` e `imu_valid` siempre aparecen. `imu_valid` exige
acelerómetro, giroscopio y magnetómetro válidos; no depende del barómetro.
También aparecen `bh1750_valid`, `sd_valid`, `sim_valid` y `gprs_connected`.
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
  "schema_version": 2,
  "device_id": "bus_iot_prototype_01",
  "uptime_ms": 12000,
  "dht_valid": true,
  "gps_valid": false,
  "accel_valid": false,
  "gyro_valid": false,
  "mag_valid": false,
  "baro_valid": false,
  "imu_valid": false,
  "bh1750_valid": false,
  "sd_valid": true,
  "sim_valid": false,
  "gprs_connected": false,
  "temperature_c": 25.0,
  "humidity_percent": 60.0
}
```

No se serializan `NaN`, `null` ni valores anteriores obsoletos.
