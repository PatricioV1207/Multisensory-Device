# Configuración de HiveMQ Cloud

VehicleSense usa HiveMQ Cloud únicamente como transporte MQTT en tiempo real.
PostgreSQL conserva el histórico y el backend es la frontera de confianza. El
frontend nunca se conecta al cluster.

Las pantallas y capacidades de autorización cambian según el plan; confirme la
interfaz actual en la [guía oficial de HiveMQ Cloud](https://docs.hivemq.com/hivemq-cloud/quick-start-guide.html)
y en [Authentication and Authorization](https://docs.hivemq.com/hivemq-cloud/authn-authz.html).

## 1. Crear el cluster

1. Cree o abra un cluster HiveMQ Cloud.
2. Copie exactamente el hostname y el puerto MQTT TLS mostrados en Connection
   Details. El puerto habitual es 8883, pero el valor visible en el cluster es
   la fuente de verdad.
3. No use la URL `wss` para el ESP32 o backend: ambos utilizan MQTT/TLS TCP.
4. Mantenga validación de CA y hostname. No habilite conexiones inseguras para
   resolver un error de certificado.

HiveMQ Cloud solo admite conexiones seguras en su flujo documentado. El ESP32
espera NTP o GPS UTC antes del handshake para evitar validar certificados con
una fecha inventada.

## 2. Separar identidades

Cree credenciales distintas:

| Principal | Uso | Client ID |
|---|---|---|
| ESP32 físico | Una credencial por dispositivo | `vs-{device_id}-{chip}` generado por firmware |
| Backend | Suscriptor confiable y comandos | Valor estable de `VEHICLESENSE_MQTT_CLIENT_ID` |
| Simulador | Identidades `sim-*` solamente | `vehiclesense-simulator-{device_id}` |
| Diagnóstico | Cliente manual temporal | Único y revocable |

No reutilice la credencial backend en firmware. Una contraseña eliminada puede
seguir en caché brevemente según HiveMQ; fuerce desconexión o espere la ventana
indicada por el portal al rotarla.

## 3. ACL recomendadas

El plan Starter o superior permite agrupar varias permissions en un role. Para
un dispositivo `device-001` de `vehicle-001`, cree permisos QoS 1:

```text
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/telemetry
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/status
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/events
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/acoustic
PUBLISH   vehiclesense/v1/vehicles/vehicle-001/devices/device-001/command-acks
SUBSCRIBE vehiclesense/v1/vehicles/vehicle-001/devices/device-001/commands
```

Permita retained publish únicamente en `status`; telemetría, eventos, acústica
y acuses no usan retain. No conceda `#` al dispositivo.

El role backend necesita:

```text
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/telemetry
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/status
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/events
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/acoustic
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/command-acks
PUBLISH   vehiclesense/v1/vehicles/+/devices/+/commands
```

El simulador debe limitarse a cada identidad exacta `sim-vehicle-001`, `002`,
etc. MQTT no permite filtrar por prefijo dentro de un nivel, por lo que
`sim-vehicle-+` no es una ACL válida de “empieza por”. Cree permisos exactos
para el número acotado de vehículos demo o use un cluster de desarrollo
aislado si necesita `+/+`.
Si el plan contratado solo permite asociar una permission por credencial y no
puede expresar ramas distintas, un filtro por raíz del dispositivo con
publish/subscribe es una concesión aceptable solo para prototipo. Para mínimo
privilegio por rama, use un plan con múltiples permissions/roles. Las
capacidades por plan se describen en la
[matriz oficial](https://docs.hivemq.com/hivemq-platform/connect/access-management-options-per-cloud-plan.html).

## 4. Configurar clientes

Firmware, en `include/secrets.h`:

```cpp
#define HIVEMQ_HOST "cluster.example.hivemq.cloud"
#define HIVEMQ_PORT 8883
#define HIVEMQ_USERNAME "device-001-credential"
#define HIVEMQ_PASSWORD "CHANGE_ME_HIVEMQ_PASSWORD"
```

Backend, en `backend/.env` o `deploy/.env`:

```dotenv
VEHICLESENSE_MQTT_ENABLED=true
VEHICLESENSE_MQTT_HOST=cluster.example.hivemq.cloud
VEHICLESENSE_MQTT_PORT=8883
VEHICLESENSE_MQTT_USERNAME=backend-credential
VEHICLESENSE_MQTT_PASSWORD=CHANGE_ME_HIVEMQ_PASSWORD
VEHICLESENSE_MQTT_CLIENT_ID=vehiclesense-backend-production-01
VEHICLESENSE_MQTT_TOPIC_PREFIX=vehiclesense/v1
```

Deje `MQTT_CA_FILE` vacío para usar el almacén de CA del sistema. El firmware
usa el bundle CA de ESP32 y verifica el hostname.

## 5. Secuencia de aceptación

1. Registre primero `vehicle_id` y `device_id` en el backend; producción usa
   `AUTO_REGISTER_DEVICES=false`.
2. Conecte el backend. `/health/ready` debe informar MQTT conectado.
3. Cargue `vehiclesense_wifi`; confirme estado retenido `online`.
4. Verifique telemetría/acústica en backend y PostgreSQL, no solo en el Web
   Client de HiveMQ.
5. Corte WiFi de forma inesperada y confirme el LWT `offline`.
6. Restaure WiFi y compruebe reconexión, `online`, cola SD y
   `replayed=true` hasta vaciar el spool por PUBACK.
7. Publique un comando desde el backend y compruebe `command-acks`.
8. Intente con una credencial equivocada o un tópico fuera de ACL: debe ser
   rechazado.

## 6. Operación segura

- No copie credenciales al frontend, capturas, Serial Monitor o logs.
- Rote cada principal por separado y mantenga client IDs únicos.
- Vigile conexiones, desconexiones, rechazos y tasa de mensajes en HiveMQ.
- QoS 1 permite duplicados: `sample_id`, `event_id` y `command_id` hacen la
  ingestión idempotente.
- Retained status no reemplaza la detección `stale/offline` del backend.
- El broker no es almacenamiento histórico ni backup.
