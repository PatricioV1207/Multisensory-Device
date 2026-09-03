# Despliegue de referencia

El proveedor de referencia es **AWS EC2** sobre Ubuntu 24.04. El despliegue actual
está reportado como operativo y sus endpoints públicos fueron comprobados el
2026-08-31; el repositorio no almacena inventario de AWS, secretos ni logs privados.
La topología funcional no cambia y HiveMQ Cloud continúa siendo un servicio externo:

```text
ESP32 ── MQTT/TLS 8883 ──► HiveMQ Cloud ──► FastAPI ──► PostgreSQL
                                                  │
Internet ── HTTP/HTTPS/WSS ──► Nginx :80/:443 ───┤
                                ├── /      React  │
                                ├── /api   FastAPI│
                                └── /ws    FastAPI│
```

`compose.production.yml` es portable entre `linux/arm64` y `linux/amd64`; no
fija `platform`. Solo Nginx publica puertos del host. FastAPI y PostgreSQL se
mantienen en redes Docker privadas, y la red del backend conserva salida a
Internet para MQTT/TLS TCP 8883.

## Guías por proveedor

- [AWS EC2](aws/README.md): procedimiento de referencia vigente para
  `vehiclemonitorsense.me` y `www.vehiclemonitorsense.me`.
- [Oracle Cloud Infrastructure](oci/README.md): alternativa portable y
  referencia histórica; no es el proveedor vigente.

## Archivos operativos

| Archivo | Propósito |
|---|---|
| `compose.production.yml` | PostgreSQL, backend, frontend, Nginx y Certbot |
| `.env.example` | Plantilla versionable sin secretos |
| `nginx/http.conf.template` | Primer arranque y challenge ACME |
| `nginx/https.conf.template` | HTTPS, WebSocket y redirección `www` al dominio raíz |
| `scripts/bootstrap_ubuntu.sh` | Instala Docker en Ubuntu 24.04 ARM64/AMD64 |
| `scripts/deploy.sh` | Valida, construye y converge el stack |
| `scripts/update.sh` | Backup, fast-forward y despliegue de una actualización |
| `scripts/audit_runtime.sh` | Preflight de solo lectura para operación y sesión E2E |
| `scripts/backup_postgres.sh` | Backup lógico con checksum |
| `scripts/restore_postgres.sh` | Restore manual, explícito y destructivo |
| `scripts/renew-certificates.sh` | Renovación Certbot y recarga Nginx |

## Inicio rápido en un host ya preparado

```bash
cd /opt/vehiclesense
cp deploy/.env.example deploy/.env
chmod 600 deploy/.env
# Editar deploy/.env y reemplazar todos los placeholders.
./deploy/scripts/deploy.sh
```

Con `NGINX_TEMPLATE=http.conf.template`, el primer arranque se comprueba por HTTP
solo para poder emitir el certificado; no introduzca credenciales en el navegador
hasta completar Certbot y cambiar a `https.conf.template`. La guía AWS contiene el
orden exacto de DNS, primer arranque y HTTPS.

El backend ejecuta `alembic upgrade head` antes de iniciar. No use
`alembic downgrade`, `pg_restore` ni un rollback de código sin revisar antes
la compatibilidad de las migraciones y disponer de un backup verificado.

Para validar la plantilla sin iniciar contenedores:

```bash
docker compose --env-file deploy/.env.example \
  -f deploy/compose.production.yml config --quiet
```

`docker compose config` interpolado puede contener secretos cuando se usa el
archivo real; no publique su salida.
