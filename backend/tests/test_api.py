from __future__ import annotations

import json

import pytest
from fastapi import WebSocketDisconnect
from fastapi.testclient import TestClient

from tests.conftest import contract_fixture


def test_auth_roles_and_fleet_crud(api_client: TestClient, admin_headers: dict[str, str]) -> None:
    assert api_client.get("/api/v1/dashboard").status_code == 401
    vehicle = api_client.post(
        "/api/v1/vehicles",
        headers=admin_headers,
        json={"vehicle_id": "vehicle-01", "display_name": "Vehículo 01"},
    )
    assert vehicle.status_code == 201, vehicle.text
    device = api_client.post(
        "/api/v1/devices",
        headers=admin_headers,
        json={"device_id": "device-01", "vehicle_id": "vehicle-01"},
    )
    assert device.status_code == 201, device.text
    dashboard = api_client.get("/api/v1/dashboard", headers=admin_headers)
    assert dashboard.status_code == 200
    assert dashboard.json()["totals"]["vehicles"] == 1
    assert dashboard.json()["vehicles"][0]["latest_telemetry"] is None

    created = api_client.post(
        "/api/v1/auth/users",
        headers=admin_headers,
        json={
            "email": "viewer@vehiclesense.example.com",
            "password": "Viewer-Password-1234",
            "role": "viewer",
        },
    )
    assert created.status_code == 201
    viewer_login = api_client.post(
        "/api/v1/auth/login",
        json={
            "email": "viewer@vehiclesense.example.com",
            "password": "Viewer-Password-1234",
        },
    )
    viewer_headers = {"Authorization": f"Bearer {viewer_login.json()['access_token']}"}
    denied = api_client.post(
        "/api/v1/vehicles",
        headers=viewer_headers,
        json={"vehicle_id": "forbidden", "display_name": "Forbidden"},
    )
    assert denied.status_code == 403


def test_ingested_telemetry_reaches_rest_and_websocket(
    api_client: TestClient, admin_headers: dict[str, str]
) -> None:
    payload = contract_fixture("telemetry_v3_full_simulated.json")
    topic = "vehiclesense/v1/vehicles/sim-vehicle-001/devices/sim-device-001/telemetry"
    outcome = api_client.portal.call(
        api_client.app.state.ingestion.ingest, topic, json.dumps(payload)
    )
    assert outcome.accepted

    dashboard = api_client.get("/api/v1/dashboard", headers=admin_headers).json()
    assert dashboard["totals"]["vehicles"] == 1
    assert dashboard["totals"]["online"] == 1
    assert dashboard["vehicles"][0]["latest_telemetry"]["temperature_c"] == 24.7

    token = admin_headers["Authorization"].split(" ", 1)[1]
    with api_client.websocket_connect("/ws/v1/live") as socket:
        socket.send_json({"action": "authenticate", "token": token})
        assert socket.receive_json()["type"] == "connection.ready"
        socket.send_json({"action": "subscribe", "vehicle_ids": ["sim-vehicle-001"]})
        assert socket.receive_json()["type"] == "subscription.updated"
        socket.send_json({"action": "ping"})
        assert socket.receive_json() == {"type": "pong"}


def test_websocket_rejects_missing_authentication_message(api_client: TestClient) -> None:
    with pytest.raises(WebSocketDisconnect) as rejected:
        with api_client.websocket_connect("/ws/v1/live") as socket:
            socket.send_json({"action": "subscribe", "vehicle_ids": []})
            socket.receive_json()
    assert rejected.value.code == 4401


def test_health_and_command_queue_without_mqtt(
    api_client: TestClient, admin_headers: dict[str, str]
) -> None:
    assert api_client.get("/health/live").status_code == 200
    ready = api_client.get("/health/ready")
    assert ready.status_code == 200
    assert ready.json()["database"] is True

    api_client.post(
        "/api/v1/vehicles",
        headers=admin_headers,
        json={"vehicle_id": "vehicle-cmd", "display_name": "Commands"},
    )
    api_client.post(
        "/api/v1/devices",
        headers=admin_headers,
        json={"device_id": "device-cmd", "vehicle_id": "vehicle-cmd"},
    )
    response = api_client.post(
        "/api/v1/commands",
        headers=admin_headers,
        json={"device_id": "device-cmd", "command_type": "publish_status"},
    )
    assert response.status_code == 202, response.text
    assert response.json()["state"] == "queued"
