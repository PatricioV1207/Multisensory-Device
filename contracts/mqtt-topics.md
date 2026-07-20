# Tópicos MQTT de VehicleSense

Prefijo de protocolo: `vehiclesense/v1`.

```text
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/telemetry
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/status
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/events
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/acoustic
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/commands
vehiclesense/v1/vehicles/{vehicle_id}/devices/{device_id}/command-acks
```

Los identificadores se limitan a letras ASCII, números, punto, guion y guion
bajo. No pueden contener `/`, `+`, `#`, espacios ni ser vacíos.

## Dirección y entrega

| Sufijo | Publica | Consume | QoS | Retain |
|---|---|---|---:|---:|
| `telemetry` | dispositivo | backend | 1 | no |
| `status` | dispositivo/broker LWT | backend | 1 | sí |
| `events` | dispositivo | backend | 1 | no |
| `acoustic` | dispositivo | backend | 1 | no |
| `commands` | backend | dispositivo | 1 | no |
| `command-acks` | dispositivo | backend | 1 | no |

`offline` se publica como LWT retenido. Al conectar, el dispositivo publica
`online` retenido. `stale` es un estado derivado por el backend cuando no llega
telemetría dentro de la ventana configurada; no se confunde con el LWT.

## ACL mínima

Credencial de dispositivo `device-001` asignada a `vehicle-001`:

```text
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/telemetry
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/status
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/events
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/acoustic
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/command-acks
SUBSCRIBE vehiclesense/v1/vehicles/vehicle-001/devices/device-001/commands
```

El backend usa otra credencial. Puede suscribirse a las cinco ramas publicadas
por dispositivos y publicar únicamente comandos. El frontend no se conecta al
broker. No se concede `#` a un dispositivo.

## Seguridad

- MQTT sobre TLS en el puerto 8883.
- validación de CA y hostname obligatoria;
- client ID único y estable por dispositivo;
- usuario/contraseña fuera de Git;
- payload máximo y frecuencia limitados;
- comandos con `command_id`, expiración y acuse idempotente.
