# Arquitectura cloud de VehicleSense

VehicleSense usa HiveMQ Cloud como transporte en tiempo real y PostgreSQL como
almacenamiento histórico. El backend es la única frontera de confianza.

```text
ESP32 + sensores + microSD
           │ WiFi / MQTT 3.1.1 / TLS / QoS 1
           ▼
      HiveMQ Cloud
           │ suscripciones con ACL de backend
           ▼
 FastAPI ──────────────── PostgreSQL
    │ REST histórico           │
    │ WebSocket en vivo        │
    ▼                          │
 React VehicleSense ◄──────────┘
```

El frontend no abre WebSocket MQTT, no conoce credenciales HiveMQ y no usa el
broker como base de datos. REST reconstruye el estado después de una
reconexión; el WebSocket de FastAPI entrega solamente cambios en vivo.

## Responsabilidades

### ESP32

- adquiere y valida sensores sin fabricar ceros ni `NaN`;
- sirve el monitor local aun sin Internet;
- respalda JSONL y mantiene un spool offline acotado en microSD;
- publica identidad, hora válida, telemetría, estado, acústica y eventos;
- recibe únicamente comandos dirigidos a su `vehicle_id/device_id`.

### HiveMQ Cloud

- autentica por credencial separada para cada dispositivo y para el backend;
- aplica ACL de mínimo privilegio;
- conserva presencia retenida/LWT, no historial de telemetría;
- entrega QoS 1; los consumidores deben ser idempotentes.

### Backend FastAPI

- valida tópico, identidad, tamaño, JSON estricto y schema versionado;
- persiste en PostgreSQL y pone rechazos en cuarentena;
- calcula `online/stale/offline`, alertas y viajes GPS básicos;
- conserva datos acústicos honestos y su marca `simulated`;
- expone REST `/api/v1` y WebSocket `/ws/v1/live` con JWT y roles;
- confirma MQTT después de persistir para permitir reentrega ante fallos.

### PostgreSQL

- almacena usuarios, vehículos, dispositivos, telemetría, acústica, alertas,
  viajes, trayectoria, historial de estado, comandos y fallos de ingestión;
- no se expone directamente a Internet;
- se actualiza mediante migraciones Alembic y se respalda con `pg_dump`.

### Frontend

- obtiene estado inicial e histórico mediante REST;
- aplica actualizaciones WebSocket y vuelve a consultar tras reconectar;
- distingue carga, ausencia de datos, sensor inválido, stale, offline y demo;
- muestra mapas mediante OpenStreetMap/Leaflet sin credenciales MQTT.

## Producción prevista en Oracle Cloud

La topología objetivo usa una VM Ubuntu ARM64/x86_64 con contenedores para
Nginx, frontend estático, FastAPI y PostgreSQL. Solamente Nginx publica 80/443;
FastAPI y PostgreSQL permanecen en la red privada de Compose. Los certificados,
secrets, volumen PostgreSQL, health checks, backup, actualización y rollback se
documentan junto con los artefactos de despliegue.

## Fuera de esta fase

El firmware conserva el trabajo SIM800L, pero el transporte activo de esta
fase es WiFi. La conectividad celular no se depura ni se convierte en una
dependencia del backend. El clasificador acústico seguirá siendo heurístico
hasta contar con grabaciones reales etiquetadas en la instalación final.
