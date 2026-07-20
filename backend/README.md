# VehicleSense backend

Estado: contrato y arquitectura definidos; implementación prevista para la
fase 4.

## Responsabilidades

- conexión TLS a HiveMQ Cloud con credencial exclusiva del backend;
- validación de schemas MQTT v2/v3 y cuarentena de mensajes incompatibles;
- persistencia idempotente en PostgreSQL;
- cálculo de online/offline/stale;
- alertas, viajes y eventos acústicos;
- REST `/api/v1` para histórico y operaciones;
- WebSocket para actualizaciones en vivo;
- autenticación y roles `admin`, `operator`, `viewer`;
- endpoints de liveness y readiness.

## Stack acordado

- Python 3.12;
- FastAPI y Pydantic 2;
- SQLAlchemy 2 async + asyncpg;
- Alembic;
- PostgreSQL 16;
- JWT access/refresh y hash Argon2;
- cliente MQTT con TLS y reconexión.

El backend es la única capa de confianza entre HiveMQ y los navegadores. El
frontend no accede al broker ni conoce sus credenciales.

## Estructura prevista

```text
backend/
├── app/
│   ├── api/v1/
│   ├── core/
│   ├── db/
│   ├── models/
│   ├── schemas/
│   ├── services/
│   ├── mqtt/
│   └── websocket/
├── migrations/
├── tests/
├── pyproject.toml
└── Dockerfile
```

Los contratos de entrada están en [`../contracts`](../contracts/README.md).
