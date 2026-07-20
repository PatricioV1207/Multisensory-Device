# Respaldo local en microSD

`MicroSDLogger` guarda exactamente el payload MQTT v2 como JSON Lines: una
muestra JSON completa por línea. Los archivos viven en `/telemetry` y siguen
el formato `boot_000123_000.jsonl`.

El contador `boot` se conserva en NVS bajo el namespace `sd_log`. Cada archivo
rota al alcanzar 1 MiB y aumenta el segmento final. No se eliminan archivos ni
se reenvían registros antiguos automáticamente.

El payload se escribe y sincroniza antes de intentar MQTT. Si no hay SIM, GPRS
o broker, el registro local continúa. Una tarjeta ausente o con error invalida
solo `sd_valid`; el montaje se reintenta cada 30 segundos.

SPI usa SCK GPIO18, MISO GPIO19, MOSI GPIO23 y CS GPIO5. GPIO5 debe permanecer
alto durante reset. La lógica hacia ESP32 nunca debe superar 3.3 V.
