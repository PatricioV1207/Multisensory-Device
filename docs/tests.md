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
pio test -e test_local_web_json
```

## Orden obligatorio

1. `full_prototype` y `full_prototype_cellular`: comprobar compilación base.
2. Serial Monitor: verificar banner sin reinicios.
3. `test_dht11`: lectura y desconexión. 
4. `test_gps`: caracteres NMEA y fix al aire libre.
5. `test_i2c_scanner`: anotar direcciones observadas.
6. `test_adxl345`, `test_l3g4200d`, `test_hmc5883l`, `test_bmp180`.
7. `calibrate_bmp180`: calcular y guardar la referencia barométrica.
8. `test_gy801`: confirmar validez independiente.
9. `test_bh1750`: lux, desconexión y recuperación.
10. `test_microsd`: montaje, JSONL, extracción y reinserción.
11. `test_wifi`: IP, RSSI y reconexión.
12. `test_mqtt_wifi`: observar el mensaje en el broker.
13. Con fuente externa adecuada para el SIM800L: `test_sim800l_at`, después
    `test_sim800l_gprs` y confirmar APN/IP/TCP.
14. `test_sim800l_mqtt`: publicar solo el payload sintético por TCP/1883.
15. `test_sim800l_mqtt_tls`: comprobar TLS/SNI con el broker real.
16. `test_local_web`: revisar dashboard y APIs; `test_local_ota`: autenticación
    y actualización con un binario válido.
17. Ejecutar las pruebas nativas y después `full_prototype_cellular` durante al
    menos 60 minutos con fallos parciales inducidos.

## BH1750 y bus I2C

Ejecuta primero `test_i2c_scanner`. El BH1750 debe aparecer en `0x23` o `0x5C`
sin ocultar las direcciones del GY-801. En `test_bh1750`, acerca y retira una
fuente de luz, cubre el sensor y desconéctalo temporalmente. Debe cambiar
`light_lux`, marcar `valid=0` al fallar y recuperarse sin reiniciar.

## microSD

Con `test_microsd`, comprueba que se cree `/telemetry` y que cada línea del
archivo sea un objeto JSON independiente. Retira y reinserta la tarjeta para
validar el reintento. GPIO5 debe permanecer alto durante reset; usa un pull-up
de 10 kΩ si el módulo no lo garantiza. El test no borra archivos ni simula por
software una tarjeta llena: esa condición se comprueba con una tarjeta de
prueba sin espacio disponible.

## SIM800L, GPRS y MQTT

No conectes el módem hasta confirmar una fuente capaz de soportar picos de al
menos 2 A, condensador cercano y GND común. La secuencia es obligatoria:

1. `test_sim800l_at`: debe detectar módem, SIM y CSQ.
2. `test_sim800l_gprs`: debe registrar red 2G, operador, APN e IP, y mostrar
   `tcp=1` usando el host y puerto `MQTT_TEST_*`.
3. `test_sim800l_mqtt`: valida TCP/1883 con telemetría sintética.
4. `test_sim800l_mqtt_tls`: valida TLS/SNI antes de habilitar telemetría real.

Un fallo TLS no autoriza texto plano automáticamente. La integración continúa
guardando JSONL en microSD. `ALLOW_INSECURE_CELLULAR_MQTT` solo puede activarse
de forma explícita para un broker privado de laboratorio.

## Web local y OTA

Configura `LOCAL_AP_SSID`, `LOCAL_AP_PASSWORD`, `LOCAL_ADMIN_USERNAME` y
`LOCAL_ADMIN_PASSWORD` en `secrets.h`. En `test_local_web` comprueba:

- `GET /`, `/api/status` y `/api/telemetry/basic` desde un teléfono conectado
  al AP.
- Ausencia de latitud, longitud, satélites, velocidad y altitud GPS en HTML y
  en ambas APIs.
- Respuesta `401` en `/admin` con credenciales ausentes o incorrectas.

Para OTA, la primera carga se realiza por USB. Compila
`full_prototype_cellular`, abre `/admin` y sube
`.pio/build/full_prototype_cellular/firmware.bin`. Verifica primero el rechazo
de un archivo no válido y luego un binario correcto. El ESP32 solo debe
reiniciarse tras finalizar `Update.end(true)` correctamente. Conserva USB como
método de recuperación.

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
- BH1750: valores finitos y no negativos; desconectarlo no detiene el bus.
- microSD: cada muestra se guarda antes del intento MQTT; fallo o extracción no
  detienen el loop.
- SIM800L: AT, SIM, registro 2G y GPRS se distinguen como estados separados.
- MQTT celular: la prueba TCP usa datos sintéticos; la integración real queda
  bloqueada si TLS no está habilitado o no conecta.
- Web local: dashboard responsive, APIs parseables y ninguna clave GPS.
- OTA: administrador protegido, archivo inválido rechazado y calibración BMP180
  conservada después de reiniciar.
- JSON: todos los tests Unity deben finalizar en `PASSED`.
- Integración: durante 60 minutos no debe haber bloqueos, watchdog resets ni
  publicación de NaN; sin red las muestras deben seguir llegando a microSD.
