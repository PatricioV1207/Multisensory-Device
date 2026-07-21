#!/usr/bin/env python3
"""Vista documental del dashboard embebido, sin modificar ni emular el firmware.

El servidor extrae el HTML directamente de ``src/web/WebAssets.h`` y responde
con datos representativos en las dos API de solo lectura. Se utiliza únicamente
para capturar las figuras del informe; no sustituye una prueba sobre el ESP32.
"""

from __future__ import annotations

import json
import re
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "src" / "web" / "WebAssets.h"
HOST = "127.0.0.1"
PORT = 8765


def load_assets() -> tuple[bytes, bytes]:
    source = HEADER.read_text(encoding="utf-8")
    assets = re.findall(r'R"HTML\((.*?)\)HTML";', source, flags=re.DOTALL)
    if len(assets) != 2:
        raise RuntimeError("No se encontraron las dos plantillas HTML esperadas")
    # En la captura automatizada el motor headless colapsa el ancho automático
    # del ``body`` de la página de administración. La regla añadida conserva el
    # ancho máximo y la intención visual del HTML embebido sin tocar el firmware.
    assets[1] = assets[1].replace(
        "</style>",
        "body{width:min(680px,calc(100% - 40px));box-sizing:border-box}"
        "section{width:100%;box-sizing:border-box}</style>",
        1,
    )
    return tuple(asset.encode("utf-8") for asset in assets)  # type: ignore[return-value]


DASHBOARD, ADMIN = load_assets()

STATUS = {
    "api_version": 2,
    "device_id": "esp32-devkit-01",
    "vehicle_id": "bus-prototipo-01",
    "firmware_version": "0.3.0",
    "time_source": "ntp",
    "simulated": False,
    "uptime_ms": 3_726_000,
    "wifi": {
        "configured": True,
        "connected": True,
        "ap_running": True,
        "rssi_dbm": -55,
        "station_ip": "192.168.1.86",
        "ap_ip": "192.168.4.1",
    },
    "sd": {
        "mounted": True,
        "last_write_ok": True,
        "last_write_ms": 3_720_000,
        "free_bytes": 7_452_672_000,
        "used_bytes": 532_480_000,
        "total_bytes": 7_985_152_000,
        "boot_session": 17,
        "segment": 2,
    },
    "sim800l": {
        "present": False,
        "sim_ready": False,
        "network_registered": False,
        "gprs_connected": False,
    },
    "mqtt": {
        "configured": True,
        "connected": True,
        "reconnecting": False,
        "last_publish_ok": True,
        "last_publish_ms": 3_720_000,
        "last_puback_ms": 3_720_080,
        "last_acknowledged_token": 184,
        "state": 0,
    },
    "offline": {
        "ready": True,
        "queued": 0,
        "replayed": 12,
        "dropped": 0,
        "oldest_age_s": 0,
        "bytes": 0,
    },
    "ota": {"enabled": True, "in_progress": False, "last_update_ok": True},
    "alerts_available": True,
}

TELEMETRY = {
    "api_version": 2,
    "uptime_ms": 3_726_000,
    "dht_valid": True,
    "bh1750_valid": True,
    "baro_valid": True,
    "accel_valid": True,
    "gyro_valid": True,
    "mag_valid": True,
    "imu_valid": True,
    "gps_valid": False,
    "gps_stream_seen": True,
    "gps_chars_processed": 18234,
    "mic_valid": True,
    "acoustic_valid": True,
    "acoustic_alert_active": False,
    "temperature_c": 27.4,
    "humidity_percent": 58.0,
    "light_lux": 8240.0,
    "pressure_hpa": 1006.72,
    "baro_altitude_m": 10.4,
    "accel_magnitude_mps2": 9.79,
    "gyro_magnitude_rad_s": 0.03,
    "mag_magnitude_ut": 54.2,
    "satellites": 3,
    "gps_hdop": 4.8,
    "acoustic_relative_level_dbfs": -32.5,
    "acoustic_peak_dbfs": -13.8,
    "acoustic_category": "normal",
    "acoustic_confidence": 0.91,
    "acoustic_clipping": False,
}


class Handler(BaseHTTPRequestHandler):
    def send_content(self, content: bytes, content_type: str, status: int = 200) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content)

    def do_GET(self) -> None:  # noqa: N802 - API requerida por BaseHTTPRequestHandler
        path = self.path.split("?", 1)[0]
        if path == "/":
            self.send_content(DASHBOARD, "text/html; charset=utf-8")
        elif path == "/admin":
            self.send_content(ADMIN, "text/html; charset=utf-8")
        elif path == "/api/status":
            self.send_content(json.dumps(STATUS).encode(), "application/json")
        elif path == "/api/telemetry/basic":
            self.send_content(json.dumps(TELEMETRY).encode(), "application/json")
        else:
            self.send_content(b"Not found", "text/plain; charset=utf-8", 404)

    def log_message(self, format: str, *args: object) -> None:
        return


if __name__ == "__main__":
    print(f"Vista documental disponible en http://{HOST}:{PORT}", flush=True)
    ThreadingHTTPServer((HOST, PORT), Handler).serve_forever()
