# Pruebas automatizadas

`test_payload_json` ejecuta en el host las pruebas Unity del serializador y del
validador. `test_barometer_math` verifica las fórmulas barométricas con las
mediciones reales de casa y universidad. Ninguno necesita ESP32 ni sensores:

```bash
pio test -e test_payload_json
pio test -e test_barometer_math
pio test -e test_local_web_json
```

`test_local_web_json` verifica ambas APIs locales y falla si aparece un campo
GPS.

Las pruebas físicas no viven aquí: se compilan como environments de firmware y
se observan mediante Serial Monitor. Esto garantiza que prueban las mismas
clases utilizadas por `full_prototype`.
