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
pio test -e test_mqtt_topics
pio test -e test_offline_queue
pio test -e test_acoustic_classifier
```

Pruebas de software cloud sin hardware:

```bash
(cd backend && uv sync --extra dev && uv run ruff check . \
  && uv run ruff format --check . && uv run pytest -q)
(cd frontend && npm ci && npm run lint && npm run format:check \
  && npm test -- --run && npm run build)
(cd simulator && uv sync --extra dev && uv run ruff check . \
  && uv run ruff format --check . && uv run pytest -q)
(cd backend && uv run python ../contracts/validate_fixtures.py)
(cd frontend && npm audit --audit-level=moderate)
uvx pip-audit --path backend/.venv/lib/python3.13/site-packages --progress-spinner off
uvx pip-audit --path simulator/.venv/lib/python3.13/site-packages --progress-spinner off
docker compose --env-file deploy/.env.example \
  -f deploy/compose.production.yml config --quiet
```

La validación de contratos usa el entorno del backend porque incluye `jsonschema`.
El frontend no define un script `typecheck`; `npm run build` ejecuta `tsc -b` antes
de Vite.

## Orden de validación de hardware

El orden es obligatorio para las pruebas que apliquen al hardware montado. El
perfil y las pruebas SIM800L solo se incluyen cuando se monta o se evalúa la
ruta celular experimental; no son requisito para aceptar la ruta WiFi.

1. `full_prototype`: comprobar compilación base; añadir
   `full_prototype_cellular` solo al evaluar la ruta celular.
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
13. Si se evalúa SIM800L, usar una fuente externa adecuada y ejecutar
    `test_sim800l_at`, después `test_sim800l_gprs`, confirmando APN/IP/TCP.
14. En esa misma evaluación celular, `test_sim800l_mqtt`: publicar solo el
    payload sintético por TCP/1883.
15. En esa misma evaluación celular, `test_sim800l_mqtt_tls`: comprobar TLS/SNI
    con el broker real.
16. `test_local_web`: revisar dashboard y APIs; `test_local_ota`: autenticación
    y actualización con un binario válido.
17. `test_inmp441`: comprobar señal I2S, nivel relativo, clipping y respuesta
    espectral; después `collect_acoustic_features` con microSD.
18. Configurar HiveMQ y ejecutar `vehiclesense_wifi`: TLS, LWT, PUBACK,
    mensajes acústicos y reconexión.
19. Ejecutar las pruebas nativas y después `vehiclesense_wifi` durante al
    menos 60 minutos con fallos parciales inducidos.

## Prueba integral HiveMQ → cloud

Esta etapa requiere credenciales reales y PostgreSQL/Docker disponibles:

1. Ejecutar `deploy/scripts/audit_runtime.sh` en EC2 y conservar la salida fechada.
   Resolver los `FAIL`; documentar los `WARN` aceptados y las cuatro verificaciones
   manuales sin convertirlas en `PASS` automáticos.
2. Aplicar `alembic upgrade head` sobre PostgreSQL vacío y comprobar
   `upgrade → downgrade → upgrade` antes de usar datos reales.
3. Crear en HiveMQ credenciales separadas para backend, ESP32 y simulador con
   las ACL de `hivemq_cloud.md`.
4. Registrar `vehicle_id/device_id` mediante la API con auto-registro apagado.
5. Arrancar backend y comprobar `/health/live=200` y `/health/ready=200`.
6. Publicar tres vehículos desde `simulator --scenario mixed`; verificar mapa,
   estados, GPS inválido, acústica, alertas, paradas, viajes y marca Demo.
7. Activar duplicados: el número de filas no debe aumentar para el mismo
   `sample_id`. Activar inyección inválida: debe ir a cuarentena sin aparecer en
   telemetría.
8. Detener/reiniciar backend y PostgreSQL por separado; MQTT debe reconectar y
   una reentrega QoS 1 no debe duplicar datos.
9. Iniciar `vehiclesense_wifi`; confirmar que la UI nunca sustituye un sensor
   inválido por cero y que la página local sigue disponible sin Internet.
10. Cortar Internet, reiniciar ESP32 con muestras pendientes y restaurar red:
   JSONL continúa, el spool sobrevive y el backend recibe `replayed=true`.
11. Comprobar roles: viewer no modifica, operator gestiona alertas/comandos y
    admin crea usuarios/vehículos/dispositivos.
12. Abrir dashboard y detalle a escritorio y móvil; no debe haber overflow,
    errores de consola ni credenciales MQTT en los assets.
13. Ejecutar backup, verificar checksum y ensayar restauración en una base de
    prueba antes de aceptar producción.

La prueba física y la cloud son complementarias: datos sintéticos validan el
flujo, no el cableado, la precisión de sensores o el clasificador acústico.

### Evidencia física E2E básica del 2026-08-31/09-01 UTC

- ESP32-D0WD-V3 revisión 3.1, MAC eFuse `d0:ef:76:45:f6:40`.
- IDs registrados: `vehicle-001` y `esp32-d0ef7645f640`.
- Credencial HiveMQ independiente con permiso limitado a la raíz MQTT del dispositivo.
- `pio run -e vehiclesense_wifi -t upload --upload-port /dev/cu.usbserial-0001`:
  carga correcta, RAM 24.1 % y flash 58.1 % en la imagen final.
- El serial emitió telemetría v3 con IDs correctos y `simulated=false`; los sensores
  ausentes/no disponibles se conservaron como inválidos.
- Se corrigió el rechazo de una hora NTP válida causado por el desbordamiento del
  límite `2100-01-01` en `time_t` de 32 bits. El serial confirmó `time_valid=true`.
- Se sustituyó el bundle CA de Arduino sin inicializar por la CA pública ISRG Root X1
  explícita, manteniendo verificación de hostname. HiveMQ conectó por TLS.
- Cuatro muestras consecutivas recibieron PUBACK QoS 1. La API productiva devolvió el
  dispositivo `online`, firmware `0.2.0`, y 20 muestras físicas recientes; la última
  consultada fue `esp32-d0ef7645f640:13:275`.
- Este PASS cubre transporte e ingesta básica. No cubre sensores, microSD, LWT,
  deduplicación, replay, comandos ni una sesión prolongada.

## BH1750 y bus I2C

Ejecuta primero `test_i2c_scanner`. El BH1750 debe aparecer en `0x23` o `0x5C`
sin ocultar las direcciones del GY-801. En `test_bh1750`, acerca y retira una
fuente de luz, cubre el sensor y desconéctalo temporalmente. Debe cambiar
`light_lux`, marcar `valid=0` al fallar y recuperarse sin reiniciar.

Una observación histórica sin fecha ni log registró las cuatro direcciones del
GY-801 junto con `[W][Wire.cpp:301] begin(): Bus already started in Master Mode`.
Al repetir el scanner, documenta si el warning reaparece y si corresponde a una
segunda inicialización real del bus. Esa nota histórica no cuenta como PASS.

## microSD

Con `test_microsd`, comprueba que se cree `/telemetry` y que cada línea del
archivo sea un objeto JSON independiente. Retira y reinserta la tarjeta para
validar el reintento. GPIO5 debe permanecer alto durante reset; usa un pull-up
de 10 kΩ si el módulo no lo garantiza. El test no borra archivos ni simula por
software una tarjeta llena: esa condición se comprueba con una tarjeta de
prueba sin espacio disponible.

En `vehiclesense_wifi` también debe crearse `/spool`. Para validar la entrega
offline:

1. Con HiveMQ conectado, confirma que `queued` vuelve a cero solo después de
   un PUBACK.
2. Apaga el punto de acceso a Internet durante al menos tres intervalos de
   telemetría. `queued` debe crecer y los JSONL deben continuar.
3. Reinicia el ESP32 todavía sin Internet. La cola debe reconstruir el mismo
   número de archivos pendientes.
4. Restaura Internet. Las muestras deben salir en orden con el `sample_id`
   original y `replayed=true`; después del PUBACK desaparecen de `/spool`.
5. Ejecuta `pio test -e test_offline_queue` para comprobar tokens, límites,
   decisión de replay y cálculo de antigüedad sin hardware.

No se prueba el límite usando la tarjeta de trabajo: usa otra tarjeta y reduce
temporalmente `OFFLINE_QUEUE_MAX_RECORDS`. Al excederlo debe aumentar
`dropped`, conservarse el límite y continuar el loop.

## SIM800L, GPRS y MQTT

Esta sección solo aplica cuando se monta o se evalúa la ruta celular
experimental. No conectes el módem hasta confirmar una fuente capaz de soportar
picos de al menos 2 A, condensador cercano y GND común. Dentro de esa evaluación,
ejecuta la secuencia completa en este orden:

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
- Estado, coordenadas y calidad GPS como texto, sin mapa, ruta ni dependencias
  web externas.
- Respuesta `401` en `/admin` con credenciales ausentes o incorrectas.

Para OTA, la primera carga se realiza por USB. Compila
`vehiclesense_wifi`, abre `/admin` y sube
`.pio/build/vehiclesense_wifi/firmware.bin`. Verifica primero el rechazo
de un archivo no válido y luego un binario correcto. El ESP32 solo debe
reiniciarse tras finalizar `Update.end(true)` correctamente. Conserva USB como
método de recuperación.

## INMP441 y análisis acústico

Conecta el INMP441 según `wiring.md`: BCLK GPIO26, WS GPIO25, SD GPIO34 y
`L/R = GND`. Ejecuta:

```bash
pio run -e test_inmp441 -t upload
pio device monitor -b 115200
```

La salida incluye `mic`, `analysis`, nivel y pico en dBFS, clipping, crest
factor, cruces por cero, centroide, flatness, rolloff, cinco bandas, categoría
y confianza. Realiza y documenta al menos estas pruebas físicas:

1. Micrófono desconectado o `L/R` incorrecto: `mic=0`, sin inventar silencio.
2. Ambiente silencioso durante 30 s: valores finitos y estables, sin alertas.
3. Voz a distancia fija: cambio visible de nivel, cruces por cero y bandas.
4. Palmada o sonido muy cercano: sube el pico; si satura debe marcar `clip=1`.
5. Tono conocido de 1 kHz reproducido por un altavoz: el centroide y la banda
   800–2000 Hz deberían dominar de forma aproximada.
6. Desconectar y volver a conectar: el firmware no debe bloquear el resto del
   sistema; la recuperación completa puede requerir reinicio porque I2S queda
   instalado durante la sesión.

Los valores son **dBFS relativos** al rango digital del micrófono. No son dB
SPL y no permiten afirmar cumplimiento de límites legales de ruido sin una
fuente acústica calibrada y una curva de calibración del conjunto.

Si `test_inmp441` reporta `mic=0` y `-160 dBFS` pese a tener 3.3 V y continuidad
correcta, carga el diagnóstico crudo de ambos slots I2S:

```bash
pio run -e diagnose_inmp441_raw -t upload
pio device monitor -b 115200
```

La línea `DIAG_INMP441_RAW` informa lecturas DMA, fallos y, para `slot0` y
`slot1`, cantidad de muestras, valores no cero, cambios, mínimo, máximo y
amplitud absoluta media. Haz silencio y luego habla o da palmadas cerca del
micrófono. Un slot con muchos valores no cero y rango variable confirma datos
en `SD`; ambos slots completamente en cero apuntan a falta de datos en `SD`,
reloj que no llega al módulo o módulo defectuoso. Los nombres `slot0` y `slot1`
son deliberadamente neutrales: el objetivo es detectar datos aunque el canal
observado no coincida con la selección esperada.

La sesión física del 2026-09-03 encontró datos en `slot0` y cero en `slot1`. En
este ESP32 y framework, el selector mono del driver que recuperó ese slot fue
`I2S_CHANNEL_FMT_ONLY_RIGHT`, aunque el INMP441 permanece físicamente con
`L/R = GND`. Esa selección ya se aplica en `test_inmp441`,
`collect_acoustic_features` y `vehiclesense_wifi`; no cambies `L/R` a 3.3 V.

Para construir un dataset de características, inserta una microSD y ejecuta:

```bash
pio run -e collect_acoustic_features -t upload
pio device monitor -b 115200
```

Selecciona la etiqueta desde el monitor: `q` quiet, `w` wind, `n` noise, `e`
engine, `p` speech, `m` music, `h` horn, `s` siren, `t` traffic, `u` unknown y
`x` pausa.
Cada agregado válido se guarda en `/acoustic/features.jsonl`. Anota para cada
sesión el lugar, vehículo, distancia al sonido, ventanas, etiqueta y
condiciones. No se almacena audio crudo.

El environment nativo `test_acoustic_classifier` comprueba FFT, clipping,
señal ausente, agregación de energía, categorías conservadoras, duración de
alerta y contratos JSON. No sustituye la validación física ni demuestra una
precisión estadística del clasificador.

## Calibración ADXL345

El environment `test_adxl345` imprime la lectura raw y calibrada del
acelerómetro en m/s²:

- `valid`
- `accel_raw_mps2=x,y,z`
- `accel_cal_mps2=x,y,z`
- `magnitude_raw_mps2`
- `magnitude_cal_mps2`

Los valores configurados se describieron como resultado de una calibración de 6
posiciones usando `+X`, `-X`, `+Y`, `-Y`, `+Z` y `-Z`, pero el repositorio no
conserva el registro crudo fechado. Deben repetirse antes de aceptar hardware. La
corrección de offset y escala usa:

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
- microSD: cada muestra se archiva y permanece en el spool hasta PUBACK; fallo
  o extracción no detienen el loop.
- Cola offline: límites de registros/bytes, replay tras reinicio, eliminación
  únicamente por PUBACK y `sample_id` estable.
- SIM800L: AT, SIM, registro 2G y GPRS se distinguen como estados separados.
- MQTT celular: la prueba TCP usa datos sintéticos; la integración real queda
  bloqueada si TLS no está habilitado o no conecta.
- Web local: dashboard responsive, APIs parseables, GPS solo textual y ningún
  mapa o recurso externo.
- OTA: administrador protegido, archivo inválido rechazado y calibración BMP180
  conservada después de reiniciar.
- INMP441: señal válida y sensible a estímulos, clipping explícito, ninguna
  cifra llamada dB SPL y ningún audio crudo guardado.
- Clasificador acústico: se acepta como heurística experimental solo si
  conserva `unknown` ante ambigüedad; la precisión debe medirse con un dataset
  etiquetado antes de usar sus categorías como evidencia.
- JSON: todos los tests Unity deben finalizar en `PASSED`.
- Integración: durante 60 minutos no debe haber bloqueos, watchdog resets ni
  publicación de NaN; sin red las muestras deben seguir llegando a microSD.

## Línea base verificable del 30–31 de agosto de 2026

La matriz de aplicación se ejecutó el día 30 y las validaciones de despliegue y
dependencias se repitieron el día 31, con estos resultados:

| Bloque | Evidencia 2026-08-30/31 |
|---|---|
| Builds ESP32 | 24/24 environments compilados |
| Unity nativo | 34/34 casos aprobados |
| Contratos JSON | 9 fixtures válidos aceptados y 7 inválidos rechazados |
| Backend | Ruff y formato limpios; pytest 15/15, con una `StarletteDeprecationWarning` de dependencia |
| Migraciones | Alembic SQLite `upgrade head → downgrade base → upgrade head` aprobado; `alembic upgrade head --sql` generó 330 líneas de SQL PostgreSQL offline |
| Frontend | ESLint y Prettier limpios; Vitest 6/6; `npm run build` completó `tsc -b` y Vite |
| Simulador | Ruff y formato limpios; pytest 10/10 |
| Despliegue local | Compose estático, build ARM64, stack PostgreSQL/backend/frontend/Nginx, Alembic, login, aislamiento de puertos y backup/restore aprobados; HTTPS pasó `nginx -t` con certificado efímero |
| Dependencias | `npm audit` y `pip-audit` sin vulnerabilidades conocidas después de actualizar locks |
| Archivos operativos | `bash -n`, ShellCheck y Hadolint aprobados; sintaxis JSON válida |

La primera ejecución desde el Python global,
`python3 contracts/validate_fixtures.py`, falló porque ese intérprete no tenía
instalado `jsonschema`. La repetición con
`backend/.venv/bin/python contracts/validate_fixtures.py` aprobó los 9 fixtures
válidos y rechazó los 7 inválidos. Es un requisito del entorno de validación, no un
fallo contractual.

Esta línea base automatizada sigue siendo local. La generación SQL offline no abrió
una conexión PostgreSQL externa; la prueba Docker usó PostgreSQL local y MQTT sin
broker, por lo que `/health/ready` devolvió el 503 esperado. Las comprobaciones
públicas de producción y su alcance están registradas por separado en `STATUS.md`;
esta matriz no demuestra placa o sensores físicos ni sustituye una E2E cloud completa.
Consulte [`../STATUS.md`](../STATUS.md) para el corte vigente.
