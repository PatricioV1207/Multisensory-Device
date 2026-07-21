# Despliegue VehicleSense en Oracle Cloud

Esta carpeta contiene un despliegue reproducible para una VM Ubuntu de Oracle
Cloud Infrastructure (OCI). HiveMQ Cloud continúa siendo el broker externo; no
se despliega Mosquitto.

```text
Internet ── HTTPS/WSS ──► Nginx :443
                            ├── /        ► frontend estático
                            ├── /api     ► FastAPI
                            └── /ws      ► FastAPI WebSocket
                                              │
HiveMQ Cloud ◄── MQTT/TLS 8883 ───────────────┤
                                              ▼
                                      PostgreSQL privado
```

Solo Nginx publica puertos Docker. FastAPI, el frontend interno y PostgreSQL no
tienen `ports`; PostgreSQL vive además en una red Docker interna. Las imágenes
base usadas son multi-arquitectura y permiten construir en `linux/arm64` para
Ampere A1 o en `linux/amd64`.

## 1. Recursos y requisitos

- Un dominio o subdominio con registro A hacia la IP pública de la VM.
- Una VM Ubuntu 24.04 LTS. Para el prototipo se recomienda Ampere A1 con 2 OCPU,
  12 GB de RAM y al menos 50 GB de disco, sujeto a capacidad y límites de la
  cuenta. Oracle documenta `VM.Standard.A1.Flex` como elegible para Always Free,
  pero la disponibilidad no está garantizada.
- Puertos de ingreso del NSG/lista de seguridad OCI: TCP 22 solo desde la IP de
  administración, TCP 80 y TCP 443 desde Internet. No abrir 5432 ni 8000.
- Salida a Internet para HTTPS, DNS y MQTT/TLS 8883.
- Credenciales separadas de HiveMQ para el backend.

Referencias vigentes: [recursos Always Free de OCI](https://docs.oracle.com/en-us/iaas/Content/FreeTier/freetier_topic-Always_Free_Resources.htm),
[creación de instancias OCI](https://docs.oracle.com/en-us/iaas/Content/Compute/Tasks/launchinginstance.htm)
y [Docker Engine para Ubuntu](https://docs.docker.com/engine/install/ubuntu/).

## 2. Preparar Ubuntu y Docker

Instale Docker desde su repositorio oficial, no mediante el script de
conveniencia de desarrollo:

```bash
sudo apt update
sudo apt install -y ca-certificates curl git openssl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
  -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

sudo tee /etc/apt/sources.list.d/docker.sources >/dev/null <<EOF
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}")
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
EOF

sudo apt update
sudo apt install -y docker-ce docker-ce-cli containerd.io \
  docker-buildx-plugin docker-compose-plugin
sudo systemctl enable --now docker
sudo docker run --rm hello-world
```

Los comandos siguientes asumen un usuario de despliegue con acceso a Docker.
Pertenecer al grupo `docker` equivale prácticamente a acceso root; si no se
acepta ese riesgo, anteponga `sudo` a Docker y adapte los scripts.

## 3. Configuración sin secretos en Git

```bash
git clone URL_DEL_REPOSITORIO.git vehiclesense
cd vehiclesense/deploy
cp .env.example .env
chmod 600 .env
openssl rand -hex 32
openssl rand -hex 32
openssl rand -base64 24
```

Use los resultados para `POSTGRES_PASSWORD`, `VEHICLESENSE_JWT_SECRET` y la
contraseña inicial de administrador. La contraseña de PostgreSQL debe ser
hexadecimal/URL-safe porque forma parte de la URL interna. Complete dominio,
correo y credenciales HiveMQ. Mantenga inicialmente:

```dotenv
NGINX_TEMPLATE=http.conf.template
VEHICLESENSE_AUTO_REGISTER_DEVICES=false
```

Compruebe la interpolación sin mostrar el resultado completo en un registro
público, porque `docker compose config` contiene secretos:

```bash
docker compose --env-file .env -f compose.production.yml config --quiet
```

## 4. Primer arranque HTTP

```bash
docker compose --env-file .env -f compose.production.yml build --pull
docker compose --env-file .env -f compose.production.yml up -d
docker compose --env-file .env -f compose.production.yml ps
curl --fail http://SU_DOMINIO/health/live
```

El contenedor backend ejecuta `alembic upgrade head` antes de arrancar. Su
healthcheck usa liveness; `/health/ready` puede devolver 503 mientras HiveMQ no
esté conectado, sin reiniciar inútilmente el servicio.

Revise logs sin copiar credenciales:

```bash
docker compose --env-file .env -f compose.production.yml logs --tail=100 backend nginx
```

## 5. Activar HTTPS con Certbot

DNS y el puerto 80 deben funcionar antes de solicitar el certificado:

```bash
set -a
source .env
set +a

docker compose --env-file .env -f compose.production.yml --profile tls run --rm \
  certbot certonly \
  --webroot --webroot-path /var/www/certbot \
  --domain "$DOMAIN" \
  --email "$LETSENCRYPT_EMAIL" \
  --agree-tos --no-eff-email
```

Cambie en `.env`:

```dotenv
NGINX_TEMPLATE=https.conf.template
```

Luego recree solamente el proxy:

```bash
docker compose --env-file .env -f compose.production.yml up -d --force-recreate nginx
curl --fail https://SU_DOMINIO/health/live
```

`https.conf.template` redirige HTTP a HTTPS, admite WebSocket, limita intentos
de login, añade encabezados de seguridad y sirve el challenge de renovación.
El JWT WebSocket se envía en el primer frame autenticado, no en la URL.

Programe la renovación diaria; Certbot solo renueva cuando corresponde:

```cron
17 03 * * * /opt/vehiclesense/deploy/scripts/renew-certificates.sh >> /var/log/vehiclesense-certbot.log 2>&1
```

## 6. Permisos HiveMQ y alta de dispositivos

El usuario del backend necesita:

```text
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/telemetry
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/status
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/events
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/acoustic
SUBSCRIBE vehiclesense/v1/vehicles/+/devices/+/command-acks
PUBLISH   vehiclesense/v1/vehicles/+/devices/+/commands
```

Las ACL del ESP32 y del simulador deben limitarse a su propia identidad; véase
`contracts/mqtt-topics.md`. El backend rechaza por defecto identidades no
registradas. Después de iniciar sesión como administrador, cree primero el
vehículo y luego el dispositivo usando los identificadores exactos del firmware:

```bash
TOKEN=$(curl --fail --silent https://SU_DOMINIO/api/v1/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"email":"ADMIN_EMAIL","password":"ADMIN_PASSWORD"}' \
  | jq -r .access_token)

curl --fail https://SU_DOMINIO/api/v1/vehicles \
  -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
  -d '{"vehicle_id":"vehicle-001","display_name":"Vehículo 001"}'

curl --fail https://SU_DOMINIO/api/v1/devices \
  -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
  -d '{"device_id":"device-001","vehicle_id":"vehicle-001"}'
unset TOKEN
```

Instale `jq` o realice estas llamadas con un cliente HTTP equivalente. Una vez
creado el administrador, vacíe `VEHICLESENSE_BOOTSTRAP_ADMIN_PASSWORD` en `.env`
y recree el backend; la cuenta persistida no se elimina.

## 7. Backup y restauración

Crear un backup lógico consistente:

```bash
./scripts/backup-postgres.sh
```

Se generan un `.dump` y su `.sha256` con permisos restrictivos. Copie ambos a
almacenamiento cifrado fuera de la VM y ensaye la restauración periódicamente.
No se aplica retención o borrado automático.

Restaurar reemplaza los objetos actuales de la base y detiene temporalmente el
backend. Haga antes una copia del estado actual:

```bash
CONFIRM_RESTORE=vehiclesense \
  ./scripts/restore-postgres.sh /ruta/vehiclesense_YYYYMMDDTHHMMSSZ.dump
```

La restauración no reemplaza `.env`, certificados ni imágenes.

## 8. Actualización y rollback

Antes de actualizar:

```bash
./scripts/backup-postgres.sh
git rev-parse HEAD > backups/pre-update-git-revision.txt
git fetch --tags
git checkout TAG_O_COMMIT_VALIDADO
export VEHICLESENSE_IMAGE_TAG=$(git rev-parse --short HEAD)
docker compose --env-file .env -f compose.production.yml build --pull
docker compose --env-file .env -f compose.production.yml up -d --remove-orphans
docker compose --env-file .env -f compose.production.yml ps
curl --fail https://SU_DOMINIO/health/ready
```

Para rollback, vuelva al commit guardado, reconstruya con otro
`VEHICLESENSE_IMAGE_TAG` y ejecute `up -d`. Si la actualización incorporó una
migración incompatible con el código anterior, restaure también el backup
previo; no use `alembic downgrade` a ciegas en producción.

## 9. Operación y seguridad

- Verifique `docker compose ps`, espacio de disco, backup externo y
  `/health/ready` con monitoreo independiente.
- No exponga 5432, 8000 ni 8080 en OCI o en Docker.
- Las reglas publicadas por Docker pueden eludir reglas UFW; Docker recomienda
  gestionar restricciones adicionales en `DOCKER-USER`. La primera barrera aquí
  es el NSG de OCI y la segunda es que Compose solo publica 80/443.
- Proteja `.env`, el directorio Certbot y los backups. No envíe su contenido a
  tickets o logs.
- Use usuarios HiveMQ distintos para backend, firmware y simulador, con ACL de
  mínimo privilegio.
- Aplique actualizaciones del host y reconstruya imágenes periódicamente.
- Esta topología es de una sola VM: no ofrece alta disponibilidad. Certificados,
  copias externas, DNS y vigilancia operativa siguen siendo responsabilidad del
  administrador.
- La capacidad Always Free puede no estar disponible y sus límites pueden
  cambiar; confirme siempre en la consola de OCI antes de crear recursos.
