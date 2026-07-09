# Pruebas automatizadas

`test_payload_json` ejecuta en el host las pruebas Unity del serializador y del
validador. No necesita ESP32 ni sensores:

```bash
pio test -e test_payload_json
```

Las pruebas físicas no viven aquí: se compilan como environments de firmware y
se observan mediante Serial Monitor. Esto garantiza que prueban las mismas
clases utilizadas por `full_prototype`.

