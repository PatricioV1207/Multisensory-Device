# Migración de telemetría v2 a v3

## Compatibilidad

La v3 conserva sin cambiar las unidades ni el significado de estos campos v2:

```text
device_id, uptime_ms,
temperature_c, humidity_percent,
latitude, longitude, altitude_m, speed_kmh, satellites,
accel_x, accel_y, accel_z,
gyro_x, gyro_y, gyro_z,
mag_x, mag_y, mag_z,
pressure_hpa, sea_level_pressure_hpa,
baro_temperature_c, baro_altitude_m,
light_lux, gsm_csq,
dht_valid, gps_valid, accel_valid, gyro_valid, mag_valid,
baro_valid, imu_valid, bh1750_valid, sd_valid,
sim_valid, gprs_connected
```

Las calibraciones físicas existentes no cambian. `accel_x/y/z` continúan siendo
aceleración calibrada en m/s² y `pressure_hpa` continúa siendo presión local
corregida en hPa.

## Campos nuevos v3

| Campo | Regla |
|---|---|
| `vehicle_id` | identidad del vehículo, independiente del dispositivo |
| `boot_id` | contador persistente de arranque |
| `sequence` | contador de muestra dentro del arranque |
| `sample_id` | `{device_id}:{boot_id}:{sequence}` |
| `time_valid` | indica si existe tiempo UTC confiable |
| `measured_at` | RFC 3339 UTC; omitido si `time_valid=false` |
| `replayed` | conserva la distinción live/replay |
| `simulated` | separa productores de demostración |
| `wifi_connected`, `wifi_rssi_dbm` | diagnóstico del transporte activo |
| `mqtt_connected` | estado en el instante de la muestra |
| `offline_*` | profundidad, replay y pérdidas de la cola |
| `mic_valid`, `acoustic_valid` | salud del INMP441 y del análisis |
| campos `acoustic_*` | resumen relativo, no dB SPL |

## Ingesta de v2

El backend acepta v2 durante la transición, valida sus rangos y guarda el
payload original. No convierte `uptime_ms` en fecha. Si el dispositivo está
asignado a un vehículo, la relación se obtiene de PostgreSQL. Si no existe
asignación, el mensaje queda en cuarentena operativa y no se inventa un
`vehicle_id`.

La retirada del adaptador v2 requiere una métrica que demuestre que ningún
dispositivo activo lo usa y una migración reversible.
