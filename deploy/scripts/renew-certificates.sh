#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ENV_FILE="${ENV_FILE:-$DEPLOY_DIR/.env}"
COMPOSE_FILE="$DEPLOY_DIR/compose.production.yml"

docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" --profile tls run --rm \
  certbot renew --webroot --webroot-path /var/www/certbot --quiet
docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" exec nginx nginx -s reload
