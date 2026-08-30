#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run this script as root: sudo $0" >&2
  exit 1
fi

if [[ ! -r /etc/os-release ]]; then
  echo "Cannot identify the operating system." >&2
  exit 1
fi

# shellcheck disable=SC1091
source /etc/os-release
if [[ "${ID:-}" != "ubuntu" || "${VERSION_ID:-}" != "24.04" ]]; then
  echo "VehicleSense production expects Ubuntu 24.04; found ${PRETTY_NAME:-unknown}." >&2
  exit 1
fi

case "$(dpkg --print-architecture)" in
  arm64 | amd64) ;;
  *)
    echo "Only Ubuntu arm64 and amd64 hosts are supported." >&2
    exit 1
    ;;
esac

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y ca-certificates curl git jq openssl

if ! command -v docker >/dev/null 2>&1 || ! docker compose version >/dev/null 2>&1; then
  apt-get remove -y \
    docker.io docker-compose docker-compose-v2 docker-doc podman-docker \
    containerd runc 2>/dev/null || true

  install -m 0755 -d /etc/apt/keyrings
  curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
    -o /etc/apt/keyrings/docker.asc
  chmod a+r /etc/apt/keyrings/docker.asc

  cat >/etc/apt/sources.list.d/docker.sources <<EOF
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: ${UBUNTU_CODENAME:-$VERSION_CODENAME}
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
EOF

  apt-get update
  apt-get install -y docker-ce docker-ce-cli containerd.io \
    docker-buildx-plugin docker-compose-plugin
fi

systemctl enable --now docker

deploy_user="${DEPLOY_USER:-${SUDO_USER:-ubuntu}}"
if id "$deploy_user" >/dev/null 2>&1; then
  usermod -aG docker "$deploy_user"
  echo "Docker access granted to $deploy_user. Reconnect the SSH session before using it."
else
  echo "User $deploy_user does not exist; Docker group membership was not changed." >&2
fi

docker version --format 'Docker Engine {{.Server.Version}}'
docker compose version
echo "Ubuntu bootstrap completed."
