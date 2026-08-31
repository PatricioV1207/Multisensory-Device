# Despliegue en AWS EC2

Esta guía despliega toda la plataforma cloud en una única
instancia EC2 Ubuntu 24.04, manteniendo HiveMQ Cloud como broker MQTT externo.
No contiene credenciales ni ejecuta operaciones sobre una cuenta AWS. Es un
procedimiento de referencia: el repositorio no demuestra que la instancia, DNS,
certificados, PostgreSQL o broker estén aprovisionados o validados.

## 1. Arquitectura resultante

```text
ESP32 ── MQTT/TLS TCP 8883 ──► HiveMQ Cloud
                                      │
                                      ▼
Internet ──► Elastic IP ──► Nginx :80/:443
                               ├── /      ──► React :8080 privado
                               ├── /api   ──► FastAPI :8000 privado
                               └── /ws    ──► FastAPI WebSocket
                                                   │
                                                   ▼
                                            PostgreSQL :5432
                                            red Docker interna
```

El frontend, FastAPI y PostgreSQL no publican puertos del host. La instancia
necesita salida a Internet para descargar imágenes, renovar certificados y
conectar FastAPI con HiveMQ Cloud por MQTT/TLS 8883.

## 2. Crear la instancia EC2

En AWS Console abra **EC2 → Launch instance** y configure:

| Opción | Valor inicial |
|---|---|
| Name | `vehiclesense-production` |
| AMI | Ubuntu Server 24.04 LTS **ARM64** |
| Instance type | `t4g.small` (2 vCPU, 2 GiB, Graviton/ARM64) |
| Key pair | Una llave nueva o existente, descargada una sola vez |
| Root volume | gp3, 30 GiB como mínimo |
| Public IP | Temporal durante creación; luego asociar Elastic IP |

Si se usa una instancia x86_64, seleccione la AMI Ubuntu 24.04 AMD64. No cambie
Compose: las imágenes base y los Dockerfiles son multi-arquitectura.

`t4g.small` es apropiada para comenzar, pero 2 GiB pueden ser ajustados durante
los builds de Node/Python. Agregue 2–4 GiB de swap y supervise memoria, disco y
CPU. Si hay OOM o latencia sostenida, cambie a `t4g.medium` sin alterar el
despliegue.

### Compatibilidad ARM64

No existe un `platform:` fijado en Compose ni en los Dockerfiles. Las imágenes
base utilizadas publican variantes ARM64:

| Imagen | Uso | Evidencia |
|---|---|---|
| `postgres:17-alpine` | Base de datos | [tag oficial arm64v8](https://hub.docker.com/r/arm64v8/postgres/) |
| `python:3.13-slim` | Backend | [tags oficiales](https://hub.docker.com/_/python/tags?name=3.13-slim) |
| `ghcr.io/astral-sh/uv:0.11.15` | Instalador Python | [plataformas soportadas](https://docs.astral.sh/uv/reference/policies/platforms/) |
| `node:24-alpine` | Build React | [tag oficial](https://hub.docker.com/_/node/tags?name=24-alpine) |
| `nginx:1.30.4-alpine` | Frontend y proxy | [tags oficiales](https://hub.docker.com/_/nginx/tags?name=1.30.4-alpine) |
| `certbot/certbot:v5.7.0` | Certificados | [tag multi-arquitectura](https://hub.docker.com/r/certbot/certbot/tags/) |

La comprobación definitiva es construir en la propia instancia Graviton con
`deploy.sh`. Antes de una actualización puede volver a revisar los manifests:

```bash
docker buildx imagetools inspect postgres:17-alpine
docker buildx imagetools inspect python:3.13-slim
docker buildx imagetools inspect node:24-alpine
docker buildx imagetools inspect nginx:1.30.4-alpine
docker buildx imagetools inspect certbot/certbot:v5.7.0
docker buildx imagetools inspect ghcr.io/astral-sh/uv:0.11.15
```

### Security Group

Cree un Security Group dedicado con estas reglas de entrada:

| Tipo | Puerto | Origen |
|---|---:|---|
| SSH | 22/TCP | Su IP pública actual `/32` |
| HTTP | 80/TCP | `0.0.0.0/0` y, si usa IPv6, `::/0` |
| HTTPS | 443/TCP | `0.0.0.0/0` y, si usa IPv6, `::/0` |

No abra 5432, 8000, 8080 ni 8883 como entrada. Mantenga salida habilitada; el
8883 es una conexión iniciada desde FastAPI hacia HiveMQ, no un servicio
entrante. Restrinja SSH de nuevo si cambia su IP.

## 3. Elastic IP y DNS Namecheap

1. En **EC2 → Elastic IP addresses**, asigne una Elastic IP de la misma región.
2. Asóciela a `vehiclesense-production`.
3. Anote la dirección únicamente en AWS y Namecheap; no la escriba en Git.
4. En Namecheap abra **Domain List → Manage → Advanced DNS**.
5. Cree o reemplace estos registros:

| Tipo | Host | Value | TTL |
|---|---|---|---|
| A Record | `@` | Elastic IP | Automatic |
| A Record | `www` | Elastic IP | Automatic |

Elimine registros de parking que compitan por `@` o `www`. Compruebe la
propagación desde su computador:

```bash
dig +short vehiclemonitorsense.me A
dig +short www.vehiclemonitorsense.me A
```

Ambos deben devolver la Elastic IP antes de solicitar el certificado. AWS
cobra las direcciones IPv4 públicas, incluidas las Elastic IP; revise el costo
vigente y libere recursos que ya no use.

## 4. Preparar Ubuntu y Docker

Conecte por SSH usando el usuario de la AMI:

```bash
chmod 600 /ruta/vehiclesense-aws.pem
ssh -i /ruta/vehiclesense-aws.pem ubuntu@ELASTIC_IP
```

Prepare `/opt`, clone el repositorio y ejecute el bootstrap versionado:

```bash
sudo install -d -o ubuntu -g ubuntu /opt/vehiclesense
git clone https://github.com/PatricioV1207/Multisensory-Device.git /opt/vehiclesense
cd /opt/vehiclesense
sudo DEPLOY_USER=ubuntu ./deploy/scripts/bootstrap_ubuntu.sh
exit
```

Vuelva a entrar por SSH para que se aplique el grupo `docker` y verifique:

```bash
docker version
docker compose version
```

El grupo `docker` concede privilegios equivalentes a root. Si no acepta ese
riesgo, no agregue el usuario al grupo y adapte los comandos para usar `sudo`.

### Swap recomendado para `t4g.small`

Ejecute una sola vez y compruebe que no exista swap antes de crearla:

```bash
swapon --show
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
free -h
```

No repita el bloque si `/swapfile` ya figura en `/etc/fstab`.

## 5. Configurar variables sin versionar secretos

```bash
cd /opt/vehiclesense
cp deploy/.env.example deploy/.env
chmod 600 deploy/.env
openssl rand -hex 32
openssl rand -hex 32
openssl rand -base64 24
nano deploy/.env
```

Use valores diferentes para PostgreSQL, JWT y administrador. La contraseña de
PostgreSQL debe ser hexadecimal/URL-safe porque forma parte de una URL interna.
Complete como mínimo:

- `DOMAIN=vehiclemonitorsense.me`;
- `WWW_DOMAIN=www.vehiclemonitorsense.me`;
- correo de Let's Encrypt;
- PostgreSQL y JWT;
- CORS con ambos orígenes HTTPS;
- administrador inicial;
- todas las variables `VEHICLESENSE_MQTT_*` entregadas por HiveMQ Cloud.

Mantenga para el primer arranque:

```dotenv
NGINX_TEMPLATE=http.conf.template
VEHICLESENSE_AUTO_REGISTER_DEVICES=false
```

Valide sin imprimir la configuración interpolada:

```bash
docker compose --env-file deploy/.env \
  -f deploy/compose.production.yml config --quiet
```

No copie `deploy/.env`, certificados ni backups a Git, chats o tickets.

## 6. Primer despliegue y migraciones Alembic

```bash
cd /opt/vehiclesense
./deploy/scripts/deploy.sh
docker compose --env-file deploy/.env \
  -f deploy/compose.production.yml ps
curl --fail -H 'Host: vehiclemonitorsense.me' http://127.0.0.1/healthz
```

El entrypoint del backend ejecuta automáticamente `alembic upgrade head` y no
arranca la API si la migración falla. Revise el estado sin modificar datos:

```bash
docker compose --env-file deploy/.env \
  -f deploy/compose.production.yml exec backend alembic current
docker compose --env-file deploy/.env \
  -f deploy/compose.production.yml logs --tail=100 backend nginx
```

Deténgase ante una migración fallida o incompatible. No fuerce revisiones, no
borre el volumen y no ejecute downgrades a ciegas.

## 7. HTTPS con Certbot

Cuando ambos registros DNS resuelvan la Elastic IP y el puerto 80 responda:

```bash
cd /opt/vehiclesense/deploy
set -a
source .env
set +a

docker compose --env-file .env -f compose.production.yml --profile tls run --rm \
  certbot certonly \
  --webroot --webroot-path /var/www/certbot \
  --cert-name "$DOMAIN" \
  --domain "$DOMAIN" --domain "$WWW_DOMAIN" \
  --email "$LETSENCRYPT_EMAIL" \
  --agree-tos --no-eff-email
```

Edite `deploy/.env`:

```dotenv
NGINX_TEMPLATE=https.conf.template
```

Recree Nginx y pruebe dominio, API y WebSocket:

```bash
docker compose --env-file .env -f compose.production.yml \
  up -d --force-recreate nginx
curl --fail https://vehiclemonitorsense.me/health/live
curl -I https://www.vehiclemonitorsense.me/
```

`www` debe redirigir al dominio raíz. Para renovación automática:

```bash
sudo crontab -e
```

```cron
17 03 * * * /opt/vehiclesense/deploy/scripts/renew-certificates.sh >> /var/log/vehiclesense-certbot.log 2>&1
```

## 8. HiveMQ Cloud

HiveMQ permanece fuera de EC2. Use un usuario exclusivo del backend y TLS en
TCP 8883. El backend necesita suscribirse a telemetría, estado, eventos,
acústica y acuses; solo debe publicar comandos. Las ACL exactas están en
[`../../contracts/mqtt-topics.md`](../../contracts/mqtt-topics.md).

Confirme en `deploy/.env`:

```dotenv
VEHICLESENSE_MQTT_HOST=CLUSTER.s1.eu.hivemq.cloud
VEHICLESENSE_MQTT_PORT=8883
VEHICLESENSE_MQTT_USERNAME=USUARIO_BACKEND
VEHICLESENSE_MQTT_PASSWORD=SECRETO_BACKEND
VEHICLESENSE_MQTT_CLIENT_ID=vehiclesense-backend-production-01
VEHICLESENSE_MQTT_TOPIC_PREFIX=vehiclesense/v1
```

No reutilice las credenciales del ESP32. La configuración de firmware, schemas,
tópicos y payloads no cambia durante esta migración de proveedor.

Después del primer acceso, registre en el backend el vehículo y el dispositivo
con los `vehicle_id` y `device_id` exactos usados por firmware y ACL. El backend
rechaza identidades no registradas porque
`VEHICLESENSE_AUTO_REGISTER_DEVICES=false`. Cuando confirme la cuenta inicial,
vacíe ambas variables bootstrap y vuelva a ejecutar `deploy.sh`:

```dotenv
VEHICLESENSE_BOOTSTRAP_ADMIN_EMAIL=
VEHICLESENSE_BOOTSTRAP_ADMIN_PASSWORD=
```

La cuenta ya persistida no se elimina. No habilite auto-registro como atajo en
producción.

## 9. Actualización, backup y restore

### Actualización normal

El script solo acepta un fast-forward limpio. Antes de cambiar código genera
un backup y guarda la revisión previa:

```bash
cd /opt/vehiclesense
./deploy/scripts/update.sh main
```

### Backup manual

```bash
cd /opt/vehiclesense
./deploy/scripts/backup_postgres.sh
```

Copie el `.dump` y `.sha256` a almacenamiento cifrado fuera de EC2. El script
no aplica retención ni borra archivos automáticamente.

### Restore

Restore reemplaza objetos existentes y detiene temporalmente FastAPI. Úselo
solo en una ventana aprobada, después de crear otro backup:

```bash
CONFIRM_RESTORE=vehiclesense \
  ./deploy/scripts/restore_postgres.sh /ruta/backup.dump
```

### Rollback de código

1. Lea `deploy/backups/pre-update-git-revision.txt`.
2. Verifique que las migraciones del código anterior sean compatibles.
3. Si no lo son, deténgase y planifique restaurar el backup previo.
4. Cambie al commit validado, defina un tag de imagen distinto y ejecute
   `deploy/scripts/deploy.sh`.

No automatice un restore ni un downgrade Alembic como reacción a un build
fallido: ambos pueden perder datos creados después de la actualización.

## 10. Operación y comprobaciones

```bash
docker compose --env-file deploy/.env -f deploy/compose.production.yml ps
docker compose --env-file deploy/.env -f deploy/compose.production.yml logs --tail=100
docker system df
df -h
free -h
curl --fail https://vehiclemonitorsense.me/health/live
curl --fail https://vehiclemonitorsense.me/health/ready
```

`/health/live` verifica el proceso. `/health/ready` puede devolver 503 si
PostgreSQL o HiveMQ no están listos y debe usarse para diagnóstico, no como
causa automática de reinicio.

La instancia única no ofrece alta disponibilidad. Elastic IP, DNS, EBS,
backups externos, parches Ubuntu, renovación TLS, monitoreo y rotación de
secretos siguen siendo responsabilidades operativas.

## Referencias oficiales

- [Parámetros de lanzamiento EC2](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-instance-launch-parameters.html)
- [Elastic IP y direcciones IPv4 públicas](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/elastic-ip-addresses-eip.html)
- [Reglas recomendadas de Security Groups](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/security-group-rules-reference.html)
- [Docker Engine para Ubuntu](https://docs.docker.com/engine/install/ubuntu/)
- [Registros A en Namecheap](https://www.namecheap.com/support/knowledgebase/article.aspx/319/2237/how-can-i-set-up-an-a-address-record-for-my-domain/)
- [Certbot en modo webroot](https://eff-certbot.readthedocs.io/en/stable/using.html)
