#!/bin/sh
set -eu

attempt=1
until alembic upgrade head; do
  if [ "$attempt" -ge 30 ]; then
    echo "Database migration failed after $attempt attempts" >&2
    exit 1
  fi
  echo "Database not ready for migration (attempt $attempt/30)" >&2
  attempt=$((attempt + 1))
  sleep 2
done

exec uvicorn app.main:app \
  --host 0.0.0.0 \
  --port 8000 \
  --proxy-headers \
  --forwarded-allow-ips="${FORWARDED_ALLOW_IPS:-*}"
