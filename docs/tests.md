# Pruebas y validación

## Comandos comunes

Compilar, cargar y abrir monitor para un environment físico:

```bash
pio run -e test_i2c_scanner
pio run -e test_i2c_scanner -t upload
pio device monitor -b 115200
```

Pruebas de payload sin placa:

```bash
pio test -e test_payload_json
pio test -e test_barometer_math
```

## Orden obligatorio

1. `full_prototype`: comprobar compilación base.
2. Serial Monitor: verificar banner sin reinicios.
3. `test_dht11`: lectura y desconexión. 
4. `test_gps`: caracteres NMEA y fix al aire libre.
5. `test_i2c_scanner`: anotar direcciones observadas.
6. `test_adxl345`, `test_l3g4200d`, `test_hmc5883l`, `test_bmp180`.
7. `calibrate_bmp180`: calcular y guardar la referencia barométrica.
8. `test_gy801`: confirmar validez independiente.
9. `test_wifi`: IP, RSSI y reconexión.
10. `test_mqtt_wifi`: observar el mensaje en el broker.
11. Pruebas nativas y después `full_prototype` durante al menos 30 minutos.

## Calibración ADXL345

El environment `test_adxl345` imprime la lectura raw y calibrada del
acelerómetro en m/s²:

- `valid`
- `accel_raw_mps2=x,y,z`
- `accel_cal_mps2=x,y,z`
- `magnitude_raw_mps2`
- `magnitude_cal_mps2`

Los valores actuales de calibración provienen de una calibración física de 6
posiciones usando las orientaciones `+X`, `-X`, `+Y`, `-Y`, `+Z` y `-Z`.
Corrigen offset y escala por eje mediante:

```text
accel_cal = (accel_raw - offset) * scale
```

En reposo, la magnitud calibrada debería quedar cerca de `9.80665 m/s²`. Esta
calibración no corrige desalineación mecánica, rotación entre ejes, vibración
estructural ni calibración avanzada por matriz.

## Calibración BMP180

El BMP180 mide presión local. La altitud se calcula respecto a una presión de
referencia a nivel del mar, que cambia con el clima. El fallback actual es
`1008.00 hPa`, obtenido de estas mediciones:

- Casa: `1006.6525 hPa` a `10 m` implica `1007.85 hPa` a nivel del mar.
- Universidad: `998.015 hPa` a `85 m` implica `1008.13 hPa`.

Con el fallback, esas muestras producen aproximadamente `11.3 m` y `83.9 m`.
No debe asumirse que `1008.00 hPa` será correcto todos los días: cerca del
nivel del mar, un cambio de `1 hPa` desplaza la altitud alrededor de `8 m`.

### Calibrar y guardar en NVS

1. Configura `BARO_CALIBRATION_KNOWN_ALTITUDE_M` con la altitud real del sitio,
   por ejemplo `10.00F` en casa o `85.00F` en la universidad.
2. Deja el dispositivo quieto. Para usar GPS, colócalo al aire libre con cielo
   visible.
3. Compila, carga y abre el monitor:

   ```bash
   pio run -e calibrate_bmp180 -t upload
   pio device monitor -b 115200
   ```

4. Espera al menos 60 segundos y 30 muestras barométricas. El GPS requiere al
   menos 6 satélites, `HDOP <= 2.5`, velocidad `<= 1 km/h` y dispersión vertical
   `<= 10 m`.
5. Revisa los candidatos impresos y envía una sola letra:

   - `k`: guardar el candidato basado en la altitud conocida, recomendado.
   - `g`: guardar el candidato GPS si superó todos los controles.
   - `r`: borrar la calibración NVS y volver al fallback.

6. Vuelve a cargar `test_bmp180` o `full_prototype`. La salida debe conservar
   `calibration_source=known_altitude` o `gps` después de reiniciar.

`test_bmp180` muestra `pressure_raw_hpa`, `pressure_local_hpa`,
`sea_level_pressure_hpa`, temperatura, altitud y fuente de calibración. La
presión local se calcula como:

```text
pressure_local = pressure_raw + BARO_PRESSURE_OFFSET_HPA
```

El offset permanece en cero hasta comparar el sensor con un barómetro
calibrado, simultáneamente y a la misma altura. No se debe obtener el offset
comparando con la presión normalizada de una aplicación meteorológica.

## Criterios PASS/FAIL

- DHT11: PASS cuando produce valores finitos; al desconectar debe pasar a
  inválido sin reiniciar.
- GPS: recibir bytes es independiente del fix. En exterior debe mostrar
  coordenadas y `fix=1`.
- I2C: cada dirección esperada debe aparecer o quedar documentada como riesgo.
- ADXL345: `valid=1`, ejes coherentes con la orientación física y
  `magnitude_cal_mps2` cercana a `9.80665 m/s²` en reposo.
- BMP180: la calibración persiste tras reiniciar, el candidato de altitud
  conocida reproduce el sitio con una tolerancia inicial de ±5 m y nunca se
  guarda automáticamente.
- GY-801: mover la placa debe cambiar los ejes; la ausencia de un chip no debe
  ocultar los demás.
- WiFi/MQTT: al apagar y restaurar la red debe reconectar automáticamente.
- JSON: todos los tests Unity deben finalizar en `PASSED`.
- Integración: no debe haber bloqueos, watchdog resets ni publicación de NaN.
