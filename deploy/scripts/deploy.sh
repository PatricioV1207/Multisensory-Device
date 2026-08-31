#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPOSITORY_DIR="$(cd "$DEPLOY_DIR/.." && pwd)"
ENV_FILE="${ENV_FILE:-$DEPLOY_DIR/.env}"
COMPOSE_FILE="$DEPLOY_DIR/compose.production.yml"

if [[ ! -f "$ENV_FILE" ]]; then
  echo "Environment file not found: $ENV_FILE" >&2
  echo "Copy deploy/.env.example to deploy/.env and replace every placeholder." >&2
  exit 1
fi
if ! command -v docker >/dev/null 2>&1 || ! docker compose version >/dev/null 2>&1; then
  echo "Docker Engine with the Compose plugin is required." >&2
  exit 1
fi

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

required=(
  DOMAIN WWW_DOMAIN LETSENCRYPT_EMAIL POSTGRES_DB POSTGRES_USER POSTGRES_PASSWORD
  VEHICLESENSE_JWT_SECRET VEHICLESENSE_CORS_ORIGINS
  VEHICLESENSE_MQTT_HOST VEHICLESENSE_MQTT_USERNAME
  VEHICLESENSE_MQTT_PASSWORD
)
for variable in "${required[@]}"; do
  value="${!variable:-}"
  if [[ -z "$value" || "$value" == REPLACE_* || "$value" == YOUR_* ]]; then
    echo "Missing or placeholder value in $ENV_FILE: $variable" >&2
    exit 1
  fi
done

if [[ ! "$POSTGRES_PASSWORD" =~ ^[[:xdigit:]]{32,}$ ]]; then
  echo "POSTGRES_PASSWORD must contain at least 32 hexadecimal characters." >&2
  exit 1
fi
if (( ${#VEHICLESENSE_JWT_SECRET} < 32 )); then
  echo "VEHICLESENSE_JWT_SECRET must contain at least 32 characters." >&2
  exit 1
fi
if [[ "${VEHICLESENSE_MQTT_PORT:-8883}" != "8883" ]]; then
  echo "Production HiveMQ must use MQTT/TLS TCP 8883." >&2
  exit 1
fi
if [[ "${NGINX_TEMPLATE:-http.conf.template}" != "http.conf.template" &&
      "${NGINX_TEMPLATE:-http.conf.template}" != "https.conf.template" ]]; then
  echo "NGINX_TEMPLATE must be http.conf.template or https.conf.template." >&2
  exit 1
fi

admin_email="${VEHICLESENSE_BOOTSTRAP_ADMIN_EMAIL:-}"
admin_password="${VEHICLESENSE_BOOTSTRAP_ADMIN_PASSWORD:-}"
if [[ -n "$admin_email" || -n "$admin_password" ]]; then
  if [[ -z "$admin_email" || -z "$admin_password" ||
        "$admin_email" == REPLACE_* || "$admin_password" == REPLACE_* ||
        ${#admin_password} -lt 12 ]]; then
    echo "Set a valid bootstrap admin email/password pair or leave both empty." >&2
    exit 1
  fi
fi

if [[ "$DOMAIN" != "vehiclemonitorsense.me" || "$WWW_DOMAIN" != "www.vehiclemonitorsense.me" ]]; then
  echo "Warning: configured domains differ from the expected production domains." >&2
fi

if [[ -z "${VEHICLESENSE_IMAGE_TAG:-}" || "$VEHICLESENSE_IMAGE_TAG" == "latest" ]]; then
  if git -C "$REPOSITORY_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    export VEHICLESENSE_IMAGE_TAG="$(git -C "$REPOSITORY_DIR" rev-parse --short=12 HEAD)"
  else
    export VEHICLESENSE_IMAGE_TAG="latest"
  fi
fi

compose=(docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE")
"${compose[@]}" config --quiet
"${compose[@]}" build --pull
"${compose[@]}" up -d --remove-orphans --wait --wait-timeout 240
"${compose[@]}" ps

echo "The platform is running with image tag $VEHICLESENSE_IMAGE_TAG."
echo "Check: https://$DOMAIN/health/live"
