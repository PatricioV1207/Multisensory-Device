# syntax=docker/dockerfile:1.7
FROM python:3.13-slim AS runtime

COPY --from=ghcr.io/astral-sh/uv:0.11.15 /uv /uvx /bin/

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    UV_COMPILE_BYTECODE=1 \
    UV_LINK_MODE=copy \
    PATH="/app/backend/.venv/bin:$PATH"

WORKDIR /app/backend
COPY backend/pyproject.toml backend/uv.lock ./
RUN uv sync --frozen --no-dev --no-install-project

COPY backend/app ./app
COPY backend/migrations ./migrations
COPY backend/alembic.ini backend/entrypoint.sh ./
COPY contracts /app/contracts
RUN uv sync --frozen --no-dev \
    && groupadd --system vehiclesense \
    && useradd --system --gid vehiclesense --home-dir /nonexistent vehiclesense \
    && chown -R vehiclesense:vehiclesense /app/backend

USER vehiclesense
EXPOSE 8000
HEALTHCHECK --interval=20s --timeout=5s --start-period=30s --retries=3 \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/health/live', timeout=3)" || exit 1

ENTRYPOINT ["/app/backend/entrypoint.sh"]
