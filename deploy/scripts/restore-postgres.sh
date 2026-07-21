#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: CONFIRM_RESTORE=vehiclesense $0 /path/to/backup.dump" >&2
  exit 2
fi
if [[ "${CONFIRM_RESTORE:-}" != "vehiclesense" ]]; then
  echo "Restore is destructive. Set CONFIRM_RESTORE=vehiclesense to continue." >&2
  exit 2
fi

BACKUP_FILE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
if [[ ! -r "$BACKUP_FILE" ]]; then
  echo "Backup file is not readable: $BACKUP_FILE" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ENV_FILE="${ENV_FILE:-$DEPLOY_DIR/.env}"
COMPOSE_FILE="$DEPLOY_DIR/compose.production.yml"

if [[ ! -f "$ENV_FILE" ]]; then
  echo "Environment file not found: $ENV_FILE" >&2
  exit 1
fi

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

checksum_file="$BACKUP_FILE.sha256"
if [[ -f "$checksum_file" ]]; then
  (
    cd "$(dirname "$BACKUP_FILE")"
    sha256sum --check "$(basename "$checksum_file")"
  )
else
  echo "Warning: no checksum file found at $checksum_file" >&2
fi

compose=(docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE")
"${compose[@]}" stop backend
trap '"${compose[@]}" up -d backend' EXIT

"${compose[@]}" exec -T postgres pg_restore \
  --username "$POSTGRES_USER" \
  --dbname "$POSTGRES_DB" \
  --clean \
  --if-exists \
  --no-owner \
  --no-acl <"$BACKUP_FILE"

"${compose[@]}" up -d backend
trap - EXIT
echo "Restore completed and backend restarted."
