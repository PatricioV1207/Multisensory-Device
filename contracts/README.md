# VehicleSense contracts

Este directorio es la fuente de verdad para los mensajes intercambiados entre
firmware, HiveMQ Cloud, backend y simulador.

## Versiones iniciales

| Mensaje | Schema | Propósito |
|---|---|---|
| Telemetría heredada | `telemetry-v2` | contrato exacto del firmware actual durante la migración |
| Telemetría | `telemetry-v3` | muestra periódica de sensores y diagnóstico |
| Estado | `status-v1` | presencia retenida, LWT y salud resumida |
| Acústica | `acoustic-v1` | características y clasificación agregada |
| Evento | `event-v1` | evento/alerta observado por dispositivo o backend |
| Comando | `command-v1` | instrucción backend → dispositivo |
| Acuse | `command-ack-v1` | resultado dispositivo → backend |

Los schemas usan JSON Schema Draft 2020-12 y son estrictos por versión. Un
cambio incompatible crea una versión nueva; no se modifica silenciosamente el
significado de una propiedad existente.

## Identidad y tiempo

- `vehicle_id` identifica el activo monitoreado.
- `device_id` identifica el hardware; se conserva por compatibilidad con v2.
- `boot_id` cambia en cada arranque.
- `sequence` aumenta dentro del arranque.
- `sample_id` es idempotente y no cambia durante un replay.
- `measured_at` solo se envía cuando `time_valid=true`.
- el backend agrega su propio `received_at`; el dispositivo nunca lo inventa.
- `simulated=true` es obligatorio para el productor de demostración.

## Validación local

```bash
python3 -m venv /tmp/vehiclesense-contracts
/tmp/vehiclesense-contracts/bin/pip install -r contracts/requirements-dev.txt
/tmp/vehiclesense-contracts/bin/python contracts/validate_fixtures.py
```

El validador exige que todos los archivos de `fixtures/valid` sean aceptados y
que todos los de `fixtures/invalid` sean rechazados por el schema indicado en
su campo auxiliar `_contract`.

## Compatibilidad con telemetría v2

El firmware actual sigue publicando schema v2 hasta la fase de integración
WiFi. El backend tendrá un adaptador v2 explícito. El adaptador:

1. conserva `device_id`, valores y banderas existentes;
2. obtiene `vehicle_id` desde la asignación registrada del dispositivo;
3. usa el instante de recepción como dato separado, nunca como supuesto
   instante de medición;
4. no fabrica `sample_id`, GPS, estado acústico ni valores ausentes;
5. marca el registro como legado para auditoría.

Consulta [mqtt-topics.md](mqtt-topics.md) y
[schema-v2-to-v3.md](schema-v2-to-v3.md).
