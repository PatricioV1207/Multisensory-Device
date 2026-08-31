#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPOSITORY_DIR="$(cd "$DEPLOY_DIR/.." && pwd)"
TARGET_BRANCH="${1:-main}"
ENV_FILE="${ENV_FILE:-$DEPLOY_DIR/.env}"

if [[ ! -f "$ENV_FILE" ]]; then
  echo "Environment file not found: $ENV_FILE" >&2
  exit 1
fi
if [[ -n "$(git -C "$REPOSITORY_DIR" status --porcelain)" ]]; then
  echo "Refusing to update a repository with local changes." >&2
  exit 1
fi

current_branch="$(git -C "$REPOSITORY_DIR" branch --show-current)"
if [[ "$current_branch" != "$TARGET_BRANCH" ]]; then
  echo "Expected branch $TARGET_BRANCH, found ${current_branch:-detached HEAD}." >&2
  exit 1
fi

git -C "$REPOSITORY_DIR" fetch --prune origin "$TARGET_BRANCH"
current_revision="$(git -C "$REPOSITORY_DIR" rev-parse HEAD)"
target_revision="$(git -C "$REPOSITORY_DIR" rev-parse "origin/$TARGET_BRANCH")"

if [[ "$current_revision" == "$target_revision" ]]; then
  echo "The platform is already at $current_revision."
  exit 0
fi

ENV_FILE="$ENV_FILE" "$SCRIPT_DIR/backup_postgres.sh"
mkdir -p "$DEPLOY_DIR/backups"
printf '%s\n' "$current_revision" >"$DEPLOY_DIR/backups/pre-update-git-revision.txt"

git -C "$REPOSITORY_DIR" merge --ff-only "origin/$TARGET_BRANCH"
export VEHICLESENSE_IMAGE_TAG="$(git -C "$REPOSITORY_DIR" rev-parse --short=12 HEAD)"

if ! ENV_FILE="$ENV_FILE" "$SCRIPT_DIR/deploy.sh"; then
  echo "Deployment failed. No database rollback was attempted." >&2
  echo "Review logs, then use the documented rollback with revision $current_revision." >&2
  exit 1
fi

echo "Updated $current_revision -> $target_revision."
