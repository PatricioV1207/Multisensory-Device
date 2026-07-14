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
```

## Orden obligatorio

1. `full_prototype`: comprobar compilación base.
2. Serial Monitor: verificar banner sin reinicios.
3. `test_dht11`: lectura y desconexión. 
4. `test_gps`: caracteres NMEA y fix al aire libre.
5. `test_i2c_scanner`: anotar direcciones observadas.
6. `test_adxl345`, `test_l3g4200d`, `test_hmc5883l`, `test_bmp180`.
7. `test_gy801`: confirmar validez independiente.
8. `test_wifi`: IP, RSSI y reconexión.
9. `test_mqtt_wifi`: observar el mensaje en el broker.
10. `test_payload_json` y después `full_prototype` durante al menos 30 minutos.

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

## Criterios PASS/FAIL

- DHT11: PASS cuando produce valores finitos; al desconectar debe pasar a
  inválido sin reiniciar.
- GPS: recibir bytes es independiente del fix. En exterior debe mostrar
  coordenadas y `fix=1`.
- I2C: cada dirección esperada debe aparecer o quedar documentada como riesgo.
- ADXL345: `valid=1`, ejes coherentes con la orientación física y
  `magnitude_cal_mps2` cercana a `9.80665 m/s²` en reposo.
- GY-801: mover la placa debe cambiar los ejes; la ausencia de un chip no debe
  ocultar los demás.
- WiFi/MQTT: al apagar y restaurar la red debe reconectar automáticamente.
- JSON: todos los tests Unity deben finalizar en `PASSED`.
- Integración: no debe haber bloqueos, watchdog resets ni publicación de NaN.
