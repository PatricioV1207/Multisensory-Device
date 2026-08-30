# Pruebas automatizadas

Los environments nativos ejecutan pruebas Unity sin ESP32 ni sensores:

```bash
pio test -e test_payload_json
pio test -e test_barometer_math
pio test -e test_local_web_json
pio test -e test_mqtt_topics
pio test -e test_offline_queue
pio test -e test_acoustic_classifier
```

`test_local_web_json` fija la separación real de las APIs locales:
`/api/telemetry/basic` incluye GPS textual cuando es válido y `/api/status` omite
GPS. También hay pruebas de contratos MQTT, spool y clasificador heurístico.

Las pruebas físicas no viven aquí: se compilan como environments de firmware y
se observan mediante Serial Monitor. Esto garantiza que prueban las mismas
clases utilizadas por los perfiles integrados. Los resultados nativos no prueban
cableado, sensores, microSD, WiFi, HiveMQ ni OTA físicos.
