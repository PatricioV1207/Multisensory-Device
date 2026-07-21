from __future__ import annotations

import asyncio
import json
import logging
import ssl
from typing import Any

import paho.mqtt.client as mqtt

from app.core.config import Settings
from app.services.ingestion import IngestionService

logger = logging.getLogger(__name__)

INBOUND_BRANCHES = ("telemetry", "status", "events", "acoustic", "command-acks")


class HiveMQService:
    def __init__(self, settings: Settings, ingestion: IngestionService) -> None:
        self.settings = settings
        self.ingestion = ingestion
        self.connected = False
        self.last_error: str | None = None
        self._loop: asyncio.AbstractEventLoop | None = None
        self._client: mqtt.Client | None = None

    def start(self, loop: asyncio.AbstractEventLoop) -> None:
        if not self.settings.mqtt_enabled:
            logger.info("HiveMQ integration disabled by configuration")
            return
        if not all(
            [
                self.settings.mqtt_host,
                self.settings.mqtt_username,
                self.settings.mqtt_password.get_secret_value(),
            ]
        ):
            raise RuntimeError("HiveMQ is enabled but host or credentials are missing")
        self._loop = loop
        client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=self.settings.mqtt_client_id,
            clean_session=False,
            protocol=mqtt.MQTTv311,
            reconnect_on_failure=True,
            manual_ack=True,
        )
        client.username_pw_set(
            self.settings.mqtt_username,
            self.settings.mqtt_password.get_secret_value(),
        )
        client.tls_set(
            ca_certs=str(self.settings.mqtt_ca_file) if self.settings.mqtt_ca_file else None,
            cert_reqs=ssl.CERT_REQUIRED,
            tls_version=ssl.PROTOCOL_TLS_CLIENT,
        )
        client.tls_insecure_set(False)
        client.reconnect_delay_set(min_delay=1, max_delay=30)
        client.on_connect = self._on_connect
        client.on_disconnect = self._on_disconnect
        client.on_message = self._on_message
        self._client = client
        client.connect_async(
            self.settings.mqtt_host,
            self.settings.mqtt_port,
            self.settings.mqtt_keepalive_seconds,
        )
        client.loop_start()
        logger.info(
            "HiveMQ TLS client started host=%s port=%d client_id=%s",
            self.settings.mqtt_host,
            self.settings.mqtt_port,
            self.settings.mqtt_client_id,
        )

    def stop(self) -> None:
        if self._client is None:
            return
        try:
            self._client.disconnect()
            self._client.loop_stop()
        finally:
            self.connected = False

    def publish_command(self, topic: str, payload: dict[str, Any]) -> bool:
        if self._client is None or not self.connected:
            return False
        result = self._client.publish(
            topic,
            json.dumps(payload, separators=(",", ":"), allow_nan=False),
            qos=1,
            retain=False,
        )
        return result.rc == mqtt.MQTT_ERR_SUCCESS

    def _on_connect(self, client, userdata, flags, reason_code, properties) -> None:
        del userdata, flags, properties
        if reason_code.is_failure:
            self.connected = False
            self.last_error = f"connect refused: {reason_code}"
            logger.error("HiveMQ connection refused reason=%s", reason_code)
            return
        self.connected = True
        self.last_error = None
        prefix = self.settings.mqtt_topic_prefix.rstrip("/")
        subscriptions = [
            (f"{prefix}/vehicles/+/devices/+/{branch}", 1) for branch in INBOUND_BRANCHES
        ]
        result, _ = client.subscribe(subscriptions)
        if result != mqtt.MQTT_ERR_SUCCESS:
            self.last_error = f"subscribe failed: {result}"
            logger.error("HiveMQ subscription failed rc=%d", result)
        else:
            logger.info("HiveMQ connected and subscribed to scoped VehicleSense topics")

    def _on_disconnect(self, client, userdata, disconnect_flags, reason_code, properties) -> None:
        del client, userdata, disconnect_flags, properties
        self.connected = False
        self.last_error = str(reason_code)
        logger.warning("HiveMQ disconnected reason=%s; paho backoff remains active", reason_code)

    def _on_message(self, client, userdata, message) -> None:
        del client, userdata
        if self._loop is None:
            return
        future = asyncio.run_coroutine_threadsafe(
            self.ingestion.ingest(message.topic, bytes(message.payload)),
            self._loop,
        )
        future.add_done_callback(
            lambda completed: self._finish_ingestion(completed, message.mid, message.qos)
        )

    def _finish_ingestion(self, future, message_id: int, qos: int) -> None:
        try:
            outcome = future.result()
            if outcome.accepted or outcome.error != "internal ingestion failure":
                if self._client is not None and qos > 0:
                    result = self._client.ack(message_id, qos)
                    if result != mqtt.MQTT_ERR_SUCCESS:
                        self.last_error = f"manual ack failed: {result}"
                        logger.error(
                            "MQTT manual acknowledgement failed mid=%d rc=%s",
                            message_id,
                            result,
                        )
                if not outcome.accepted:
                    logger.warning("MQTT payload rejected and quarantined error=%s", outcome.error)
                return
            self.last_error = "database ingestion failed; MQTT message left unacknowledged"
            logger.error("MQTT ingestion failed; forcing reconnect for broker redelivery")
            if self._loop is not None:
                self._loop.create_task(self._reconnect_after_ingestion_failure())
        except Exception:
            logger.exception("Unhandled MQTT ingestion callback failure")

    async def _reconnect_after_ingestion_failure(self) -> None:
        client = self._client
        if client is None:
            return
        await asyncio.to_thread(client.disconnect)
        await asyncio.sleep(1)
        try:
            await asyncio.to_thread(client.reconnect)
        except Exception as error:
            self.last_error = f"reconnect after ingestion failure: {error}"
            logger.warning("HiveMQ reconnect after database failure did not complete: %s", error)
