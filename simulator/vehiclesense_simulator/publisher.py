from __future__ import annotations

import copy
import json
import logging
import ssl
import threading
from collections import deque
from datetime import UTC, datetime

import paho.mqtt.client as mqtt

from vehiclesense_simulator.config import SimulatorSettings
from vehiclesense_simulator.generator import Envelope, VehicleGenerator

logger = logging.getLogger(__name__)


class DevicePublisher:
    def __init__(self, settings: SimulatorSettings, generator: VehicleGenerator) -> None:
        self.settings = settings
        self.generator = generator
        self.connected = threading.Event()
        self.queue: deque[Envelope] = deque()
        self.dropped = 0
        self.replayed = 0
        self._started = False
        self._lock = threading.Lock()
        self._client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=f"vehiclesense-simulator-{generator.device_id}",
            clean_session=False,
            protocol=mqtt.MQTTv311,
            reconnect_on_failure=True,
        )
        self._client.username_pw_set(settings.mqtt_username, settings.mqtt_password)
        self._client.tls_set(
            ca_certs=str(settings.mqtt_ca_file) if settings.mqtt_ca_file else None,
            cert_reqs=ssl.CERT_REQUIRED,
            tls_version=ssl.PROTOCOL_TLS_CLIENT,
        )
        self._client.tls_insecure_set(False)
        self._client.reconnect_delay_set(min_delay=1, max_delay=30)
        lwt = generator.status("offline", "unexpected_disconnect", datetime.now(UTC))
        self._client.will_set(
            lwt.topic,
            json.dumps(lwt.payload, separators=(",", ":"), allow_nan=False),
            qos=1,
            retain=True,
        )
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

    @property
    def queued(self) -> int:
        with self._lock:
            return len(self.queue)

    def start(self, timeout: float = 15.0) -> None:
        self._client.connect_async(
            self.settings.mqtt_host,
            self.settings.mqtt_port,
            keepalive=30,
        )
        self._client.loop_start()
        self._started = True
        if not self.connected.wait(timeout):
            raise RuntimeError(f"HiveMQ connection timeout for {self.generator.device_id}")

    def stop(self) -> None:
        if not self._started:
            return
        if self.connected.is_set():
            offline = self.generator.status("offline", "shutdown", datetime.now(UTC))
            self._publish_now(offline)
        self._client.disconnect()
        self._client.loop_stop()
        self.connected.clear()
        self._started = False

    def publish(self, envelope: Envelope) -> bool:
        if not self.connected.is_set() or not self._publish_now(envelope):
            if envelope.payload.get("message_type") != "status":
                self._enqueue(envelope)
            return False
        return True

    def flush(self, maximum: int = 25) -> int:
        if not self.connected.is_set():
            return 0
        completed = 0
        while completed < maximum:
            with self._lock:
                if not self.queue:
                    break
                envelope = self.queue.popleft()
            replay = self._as_replay(envelope)
            if not self._publish_now(replay):
                with self._lock:
                    self.queue.appendleft(envelope)
                break
            completed += 1
            self.replayed += 1
        return completed

    def _enqueue(self, envelope: Envelope) -> None:
        with self._lock:
            if len(self.queue) >= self.settings.queue_limit:
                self.queue.popleft()
                self.dropped += 1
            self.queue.append(copy.deepcopy(envelope))

    def _publish_now(self, envelope: Envelope, *, wait: bool = True) -> bool:
        payload = json.dumps(envelope.payload, separators=(",", ":"), allow_nan=False)
        result = self._client.publish(
            envelope.topic,
            payload,
            qos=envelope.qos,
            retain=envelope.retain,
        )
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            return False
        if envelope.qos and wait:
            result.wait_for_publish(timeout=5.0)
        return True if not wait else result.is_published()

    @staticmethod
    def _as_replay(envelope: Envelope) -> Envelope:
        payload = copy.deepcopy(envelope.payload)
        if "replayed" in payload:
            payload["replayed"] = True
        return Envelope(
            envelope.topic,
            payload,
            qos=envelope.qos,
            retain=envelope.retain,
            validate=envelope.validate,
        )

    def _on_connect(self, client, userdata, flags, reason_code, properties) -> None:
        del userdata, flags, properties
        if reason_code.is_failure:
            logger.error(
                "HiveMQ rejected simulator device=%s reason=%s",
                self.generator.device_id,
                reason_code,
            )
            return
        self.connected.set()
        client.subscribe(self.generator.branch_topic("commands"), qos=1)
        # This callback runs on Paho's network thread. Waiting for PUBACK here
        # would block the same thread that must process the acknowledgement.
        self._publish_now(
            self.generator.status("online", "connected", datetime.now(UTC)),
            wait=False,
        )
        logger.info("Simulator connected device=%s", self.generator.device_id)

    def _on_disconnect(self, client, userdata, disconnect_flags, reason_code, properties) -> None:
        del client, userdata, disconnect_flags, properties
        self.connected.clear()
        logger.warning(
            "Simulator disconnected device=%s reason=%s",
            self.generator.device_id,
            reason_code,
        )

    def _on_message(self, client, userdata, message) -> None:
        del client, userdata
        try:
            command = json.loads(bytes(message.payload))
            self.generator.registry.validate(command)
            if (
                command["device_id"] != self.generator.device_id
                or command["vehicle_id"] != self.generator.vehicle_id
            ):
                raise ValueError("command identity does not match simulator device")
            now = datetime.now(UTC)
            expires_at = datetime.fromisoformat(command["expires_at"].replace("Z", "+00:00"))
            if expires_at.tzinfo is None:
                raise ValueError("command expires_at must include a UTC offset")
            if expires_at <= now:
                acknowledgement = self.generator.command_ack(
                    command,
                    now,
                    state="expired",
                    message="Command expired before simulated execution",
                )
            else:
                acknowledgement = self.generator.command_ack(command, now)
            if not self._publish_now(acknowledgement, wait=False):
                self._enqueue(acknowledgement)
            logger.info(
                "Simulated command acknowledged device=%s command=%s state=%s",
                self.generator.device_id,
                command["command"],
                acknowledgement.payload["state"],
            )
        except (ValueError, json.JSONDecodeError, KeyError):
            logger.exception("Simulator rejected an invalid command")
