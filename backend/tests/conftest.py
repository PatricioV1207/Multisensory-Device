from __future__ import annotations

import json
from pathlib import Path

import pytest
from fastapi.testclient import TestClient
from pydantic import SecretStr

from app.core.config import REPOSITORY_ROOT, Settings
from app.main import create_app


def settings_for(database_url: str, **overrides) -> Settings:
    values = {
        "environment": "test",
        "database_url": database_url,
        "auto_create_schema": True,
        "mqtt_enabled": False,
        "auto_register_devices": True,
        "jwt_secret": SecretStr("backend-test-secret-with-at-least-32-characters"),
        "bootstrap_admin_email": "admin@vehiclesense.example.com",
        "bootstrap_admin_password": SecretStr("Correct-Horse-Battery-Staple"),
        "status_scan_seconds": 3600,
        "trip_start_duration_seconds": 10,
        "trip_stop_duration_seconds": 20,
    }
    values.update(overrides)
    return Settings(**values)


def contract_fixture(name: str) -> dict:
    path = REPOSITORY_ROOT / "contracts" / "fixtures" / "valid" / name
    payload = json.loads(path.read_text(encoding="utf-8"))
    payload.pop("_contract", None)
    return payload


@pytest.fixture
def api_client(tmp_path: Path):
    database = tmp_path / "api.db"
    app = create_app(settings_for(f"sqlite+aiosqlite:///{database}"))
    with TestClient(app) as client:
        yield client


@pytest.fixture
def admin_headers(api_client: TestClient) -> dict[str, str]:
    response = api_client.post(
        "/api/v1/auth/login",
        json={
            "email": "admin@vehiclesense.example.com",
            "password": "Correct-Horse-Battery-Staple",
        },
    )
    assert response.status_code == 200, response.text
    return {"Authorization": f"Bearer {response.json()['access_token']}"}
