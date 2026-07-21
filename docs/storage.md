# Almacenamiento y entrega offline

VehicleSense separa deliberadamente dos usos de la microSD:

1. **Archivo de auditoría:** `MicroSDLogger` guarda cada payload como una línea
   JSON independiente bajo `/telemetry`. Los archivos
   `boot_000123_000.jsonl` rotan al llegar a 1 MiB y no se eliminan
   automáticamente.
2. **Spool de entrega:** `OfflineTelemetryQueue` conserva bajo `/spool` cada
   muestra v3 pendiente hasta recibir el PUBACK de MQTT QoS 1.

El archivo JSONL no se consume al reenviar y el spool no reemplaza al archivo.
Esta separación evita confundir historial local con estado de entrega.

## Garantías y límites del spool

- Orden FIFO: siempre se intenta primero el registro más antiguo.
- El `sample_id`, el tiempo de medición y el payload original se conservan.
- Una muestra diferida, reintentada o recuperada tras reinicio se publica con
  `replayed=true`.
- El archivo del spool se elimina únicamente después del PUBACK.
- Cada registro tiene cabecera versionada y checksum FNV-1a para detectar
  truncamiento o corrupción.
- Una escritura usa `tmp.msg` y después un rename, para no aceptar archivos
  parcialmente escritos como registros válidos.
- El límite predeterminado es 256 registros, 768 KiB y 7 días. Al alcanzar
  registros o bytes se descarta explícitamente el elemento más antiguo y se
  incrementa `dropped`.
- La expiración por edad solo puede aplicarse cuando existe una hora UTC
  válida. Los límites de registros y bytes siempre se aplican.
- El replay se limita a un intento cada 250 ms y nunca bloquea la adquisición.
- Un PUBACK que tarda más de 30 s libera el turno local; el registro durable
  permanece. El backend debe deduplicar por `sample_id` porque QoS 1 permite
  duplicados.

Los contadores `queued`, `bytes`, `replayed`, `dropped` y
`oldest_age_s` aparecen en `/api/status`. `queued` y `bytes` se reconstruyen
escaneando `/spool` tras reiniciar; `replayed` y `dropped` son diagnósticos de
la ejecución actual.

Si falta la tarjeta, el firmware continúa con sensores, página local y MQTT.
Cuando HiveMQ está disponible intenta entrega directa, pero esa muestra no
tiene garantía durable. Si tampoco existe red, la pérdida se registra por
Serial. El montaje y el spool se reintentan periódicamente.

## Hardware

SPI usa SCK GPIO18, MISO GPIO19, MOSI GPIO23 y CS GPIO5. GPIO5 debe permanecer
alto durante reset, preferiblemente con pull-up de 10 kΩ. La lógica hacia el
ESP32 nunca debe superar 3.3 V.
