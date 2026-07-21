from __future__ import annotations

import asyncio
from collections import defaultdict
from typing import Any

from fastapi import WebSocket


class LiveUpdateManager:
    def __init__(self) -> None:
        self._connections: set[WebSocket] = set()
        self._vehicle_subscriptions: dict[WebSocket, set[str]] = defaultdict(set)
        self._lock = asyncio.Lock()

    async def connect(self, websocket: WebSocket) -> None:
        await websocket.accept()
        async with self._lock:
            self._connections.add(websocket)

    async def disconnect(self, websocket: WebSocket) -> None:
        async with self._lock:
            self._connections.discard(websocket)
            self._vehicle_subscriptions.pop(websocket, None)

    async def subscribe(self, websocket: WebSocket, vehicle_ids: list[str]) -> None:
        async with self._lock:
            if websocket in self._connections:
                self._vehicle_subscriptions[websocket] = set(vehicle_ids[:100])

    async def broadcast(self, message: dict[str, Any], vehicle_id: str | None = None) -> None:
        async with self._lock:
            targets = list(self._connections)
            subscriptions = {
                connection: set(self._vehicle_subscriptions.get(connection, set()))
                for connection in targets
            }
        disconnected: list[WebSocket] = []
        for connection in targets:
            selected = subscriptions[connection]
            if vehicle_id is not None and selected and vehicle_id not in selected:
                continue
            try:
                await connection.send_json(message)
            except Exception:
                disconnected.append(connection)
        for connection in disconnected:
            await self.disconnect(connection)
