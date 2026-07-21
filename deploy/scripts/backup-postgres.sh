#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ENV_FILE="${ENV_FILE:-$DEPLOY_DIR/.env}"
COMPOSE_FILE="$DEPLOY_DIR/compose.production.yml"
BACKUP_DIR="${BACKUP_DIR:-$DEPLOY_DIR/backups}"

if [[ ! -f "$ENV_FILE" ]]; then
  echo "Environment file not found: $ENV_FILE" >&2
  exit 1
fi

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

mkdir -p "$BACKUP_DIR"
chmod 700 "$BACKUP_DIR"
umask 077

timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
filename="vehiclesense_${timestamp}.dump"
temporary="$BACKUP_DIR/.${filename}.tmp"
final="$BACKUP_DIR/$filename"
trap 'rm -f "$temporary"' EXIT

docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" exec -T postgres \
  pg_dump \
    --username "$POSTGRES_USER" \
    --dbname "$POSTGRES_DB" \
    --format custom \
    --compress 9 \
    --no-owner \
    --no-acl >"$temporary"

mv "$temporary" "$final"
(
  cd "$BACKUP_DIR"
  sha256sum "$filename" >"$filename.sha256"
)
trap - EXIT

echo "Backup created: $final"
echo "Copy the .dump and .sha256 files to storage outside this VM."
