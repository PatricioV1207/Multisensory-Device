#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPOSITORY_DIR="$(cd "$DEPLOY_DIR/.." && pwd)"
ENV_FILE="${ENV_FILE:-$DEPLOY_DIR/.env}"
COMPOSE_FILE="$DEPLOY_DIR/compose.production.yml"

failures=0
warnings=0

pass() {
  echo "PASS  $*"
}

warn() {
  warnings=$((warnings + 1))
  echo "WARN  $*"
}

fail() {
  failures=$((failures + 1))
  echo "FAIL  $*"
}

require_command() {
  if command -v "$1" >/dev/null 2>&1; then
    pass "command available: $1"
  else
    fail "required command missing: $1"
  fi
}

echo "Runtime audit UTC $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "Repository: $REPOSITORY_DIR"

for command in docker curl sha256sum swapon; do
  require_command "$command"
done

if (( failures > 0 )); then
  echo "SUMMARY failures=$failures warnings=$warnings"
  exit 1
fi

if [[ ! -f "$ENV_FILE" ]]; then
  fail "environment file not found: $ENV_FILE"
  echo "SUMMARY failures=$failures warnings=$warnings"
  exit 1
fi

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

env_mode="$(stat -c '%a' "$ENV_FILE" 2>/dev/null || true)"
if [[ "$env_mode" == "600" || "$env_mode" == "400" ]]; then
  pass "environment file permissions are restrictive ($env_mode)"
else
  warn "environment file mode is ${env_mode:-unknown}; expected 600 or 400"
fi

secret_copy_count="$(
  find "$DEPLOY_DIR" -maxdepth 1 -type f -name '.env.*' \
    ! -name '.env.example' -print | wc -l | tr -d ' '
)"
if [[ "$secret_copy_count" == "0" ]]; then
  pass "no additional deploy environment copies were found"
else
  fail "found $secret_copy_count additional deploy environment file(s); remove plaintext secret copies after review"
fi

if [[ "${VEHICLESENSE_AUTO_REGISTER_DEVICES:-false}" == "false" ]]; then
  pass "automatic device registration is disabled"
else
  fail "VEHICLESENSE_AUTO_REGISTER_DEVICES must be false"
fi

if [[ -z "${VEHICLESENSE_BOOTSTRAP_ADMIN_EMAIL:-}" &&
      -z "${VEHICLESENSE_BOOTSTRAP_ADMIN_PASSWORD:-}" ]]; then
  pass "bootstrap administrator variables are empty"
else
  fail "bootstrap administrator variables remain set"
fi

if [[ "${VEHICLESENSE_MQTT_PORT:-8883}" == "8883" ]]; then
  pass "MQTT uses TLS port 8883"
else
  fail "VEHICLESENSE_MQTT_PORT is not 8883"
fi

if [[ "${VEHICLESENSE_MQTT_TOPIC_PREFIX:-}" == "vehiclesense/v1" ]]; then
  pass "MQTT topic prefix matches the contracts"
else
  fail "VEHICLESENSE_MQTT_TOPIC_PREFIX does not match vehiclesense/v1"
fi

if [[ "${NGINX_TEMPLATE:-}" == "https.conf.template" ]]; then
  pass "Nginx uses the HTTPS template"
else
  fail "NGINX_TEMPLATE is not https.conf.template"
fi

if docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" config --quiet; then
  pass "Compose configuration is valid"
else
  fail "Compose configuration is invalid"
fi

for service in postgres backend frontend nginx; do
  running_count="$(
    docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" \
      ps --status running -q "$service" 2>/dev/null | wc -l | tr -d ' ' || true
  )"
  if [[ "$running_count" == "1" ]]; then
    pass "service running: $service"
  else
    fail "service is not running exactly once: $service"
  fi
done

domain="${DOMAIN:-}"
if [[ -n "$domain" ]]; then
  live_body="$(curl --fail --silent --show-error --max-time 15 \
    "https://$domain/health/live" 2>/dev/null || true)"
  ready_body="$(curl --fail --silent --show-error --max-time 15 \
    "https://$domain/health/ready" 2>/dev/null || true)"
  if [[ "$live_body" == *'"status":"alive"'* ]]; then
    pass "public liveness endpoint responds"
  else
    fail "public liveness endpoint did not return alive"
  fi
  if [[ "$ready_body" == *'"status":"ready"'* &&
        "$ready_body" == *'"database":true'* &&
        "$ready_body" == *'"connected":true'* ]]; then
    pass "public readiness confirms PostgreSQL and MQTT"
  else
    fail "public readiness does not confirm PostgreSQL and MQTT"
  fi
else
  fail "DOMAIN is empty"
fi

swap_lines="$(swapon --noheadings --show=NAME,SIZE 2>/dev/null || true)"
if [[ -n "$swap_lines" ]]; then
  pass "swap is active"
  while IFS= read -r swap_line; do
    echo "INFO  swap $swap_line"
  done <<<"$swap_lines"
else
  warn "no active swap detected"
fi

if awk '$3 == "swap" {found=1} END {exit !found}' /etc/fstab; then
  pass "swap has a persistent /etc/fstab entry"
else
  warn "no persistent swap entry found in /etc/fstab"
fi

disk_percent="$(df -P / | awk 'NR == 2 {gsub(/%/, "", $5); print $5}')"
if [[ "$disk_percent" =~ ^[0-9]+$ ]] && (( disk_percent < 80 )); then
  pass "root filesystem usage is below 80% (${disk_percent}%)"
elif [[ "$disk_percent" =~ ^[0-9]+$ ]] && (( disk_percent < 90 )); then
  warn "root filesystem usage is elevated (${disk_percent}%)"
else
  fail "root filesystem usage is critical (${disk_percent:-unknown}%)"
fi

renewal_script="$REPOSITORY_DIR/deploy/scripts/renew-certificates.sh"
root_cron="$(sudo -n crontab -l 2>/dev/null || true)"
renewal_count="$(grep -F -c "$renewal_script" <<<"$root_cron" || true)"
if [[ "$renewal_count" == "1" ]]; then
  pass "root cron contains one certificate-renewal entry"
elif [[ "$renewal_count" == "0" ]]; then
  warn "certificate-renewal cron was not observed with passwordless sudo"
else
  fail "certificate-renewal cron appears more than once"
fi

backup_dir="$DEPLOY_DIR/backups"
latest_checksum=""
if [[ -d "$backup_dir" ]]; then
  latest_checksum="$(
    find "$backup_dir" -maxdepth 1 -type f -name '*.dump.sha256' -print |
      sort | tail -n 1
  )"
fi
if [[ -n "$latest_checksum" ]]; then
  if (cd "$backup_dir" && sha256sum --check "$(basename "$latest_checksum")"); then
    pass "latest local PostgreSQL backup checksum is valid"
  else
    fail "latest local PostgreSQL backup checksum failed"
  fi
else
  warn "no local PostgreSQL backup checksum was found"
fi

if [[ -n "$(git -C "$REPOSITORY_DIR" status --short)" ]]; then
  warn "production repository worktree is not clean"
else
  pass "production repository worktree is clean"
fi
echo "INFO  revision $(git -C "$REPOSITORY_DIR" rev-parse --short=12 HEAD)"

echo "MANUAL Verify an encrypted backup copy exists outside EC2 and record its restore test."
echo "MANUAL Verify AWS Budgets/CloudWatch billing alerts and their notification recipients."
echo "MANUAL Record dates for PostgreSQL, JWT, HiveMQ, SSH and any AWS credential rotation."
echo "MANUAL Review EC2/EBS monitoring, pending Ubuntu updates and recent service logs."
echo "SUMMARY failures=$failures warnings=$warnings"

if (( failures > 0 )); then
  exit 1
fi
