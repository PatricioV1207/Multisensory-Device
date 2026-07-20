# VehicleSense deployment

Estado: topología definida; artefactos de despliegue previstos para la fase 7.

Producción utilizará HiveMQ Cloud como broker externo y una VM Ubuntu de
Oracle Cloud cuando haya capacidad disponible.

```text
Internet → Nginx :443
             ├─ frontend estático
             ├─ /api → FastAPI
             └─ /ws  → FastAPI WebSocket
          PostgreSQL en red Docker privada
```

Artefactos futuros:

- Dockerfiles multi-stage ARM64/amd64;
- `compose.production.yml`;
- Nginx con HTTPS y proxy WebSocket;
- health checks y restart policies;
- volumen persistente PostgreSQL;
- `.env.example` sin secretos;
- scripts de backup, restore, actualización y rollback;
- guía OCI, DNS, firewall y certificados.

Solo Nginx expondrá la aplicación. PostgreSQL no tendrá un puerto público y no
se desplegará Mosquitto.
