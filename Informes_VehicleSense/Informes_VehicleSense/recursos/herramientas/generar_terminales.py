#!/usr/bin/env python3
"""Genera capturas representativas del monitor serie para el informe 2.

Estas figuras no son registros físicos. El contenido replica el formato de
``ModuleTestRunner.cpp`` y debe conservar la etiqueta de salida representativa.
"""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


OUT = Path(__file__).resolve().parents[1] / "terminales"
OUT.mkdir(parents=True, exist_ok=True)

FONT_CANDIDATES = [
    "/System/Library/Fonts/SFNSMono.ttf",
    "/System/Library/Fonts/Supplemental/Courier New.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
]


def font(size: int):
    for candidate in FONT_CANDIDATES:
        if Path(candidate).exists():
            return ImageFont.truetype(candidate, size)
    return ImageFont.load_default()


CAPTURES = {
    "terminal_gps_representativo.png": [
        "PlatformIO Device Monitor | 115200 8-N-1",
        "[BOOT] Environment: test_gps",
        "[GPS] UART2 started at 9600 baud RX=32 TX=33",
        "[TEST_GPS] fix=0 stream=1 chars=418 lat=nan lon=nan sats=3 hdop=4.80",
        "[GPS] NMEA received; waiting for a fresh position fix",
        "[TEST_GPS] fix=1 stream=1 chars=1246 lat=-2.153421 lon=-79.889765 sats=8 hdop=1.10",
        "[RESULT] PASS_SIMULATED - UART active and outdoor fix obtained",
    ],
    "terminal_microsd_representativo.png": [
        "PlatformIO Device Monitor | 115200 8-N-1",
        "[BOOT] Environment: test_microsd",
        "[SD] SPI SCK=18 MISO=19 MOSI=23 CS=5",
        "[SD] Card mounted; session=18 segment=0",
        "[TEST_MICROSD] mounted=1 last_write_ok=1 boot=18 segment=0 free_bytes=7452672000",
        "[SD] Card removed or write failed; acquisition continues",
        "[TEST_MICROSD] mounted=0 last_write_ok=0 boot=18 segment=0 free_bytes=0",
        "[SD] Card mounted again; JSONL append recovered",
        "[RESULT] PASS_SIMULATED - mount, failure flag and recovery verified",
    ],
    "terminal_wifi_mqtt_representativo.png": [
        "PlatformIO Device Monitor | 115200 8-N-1",
        "[BOOT] Environment: test_mqtt_wifi",
        "[WIFI] Connecting to configured SSID",
        "[TEST_NET] wifi=1 ip=192.168.1.86 rssi=-55 mqtt=0 state=-1",
        "[MQTT] Connected with TLS; QoS 1 session active",
        "[MQTT] Test payload published; waiting for PUBACK",
        "[TEST_NET] wifi=1 ip=192.168.1.86 rssi=-56 mqtt=1 state=0",
        "[WIFI] Link lost; retry scheduled without blocking acquisition",
        "[WIFI] Reconnected ip=192.168.1.86 rssi=-57",
        "[MQTT] PUBACK token=184 received",
        "[RESULT] PASS_SIMULATED - test_wifi and test_mqtt_wifi",
    ],
    "terminal_sim800_representativo.png": [
        "PlatformIO Device Monitor | 115200 8-N-1",
        "[BOOT] Environment: test_sim800l_gprs",
        "[SIM800] AT synchronized; model=SIMCOM_SIM800L",
        "[TEST_SIM800L] modem=1 model=SIM800L sim=1 network=0 gprs=0 operator= ip= csq=12 dbm=-89 error=0 tcp=-1 mqtt=0 state=-1",
        "[SIM800] Waiting for 2G registration; acquisition is not blocked",
        "[TEST_SIM800L] modem=1 model=SIM800L sim=1 network=1 gprs=1 operator=TEST-NET ip=10.22.8.4 csq=17 dbm=-79 error=0 tcp=1 mqtt=0 state=-1",
        "[RESULT] PARTIAL_SIMULATED - AT/GPRS/TCP OK; TLS not validated",
    ],
    "terminal_inmp441_representativo.png": [
        "PlatformIO Device Monitor | 115200 8-N-1",
        "[BOOT] Environment: test_inmp441",
        "[AUDIO] I2S microphone ready; raw audio persistence disabled",
        "[TEST_INMP441] mic=1 analysis=1 level_dbfs=-34.20 peak_dbfs=-13.70 clip=0",
        " crest=3.110 zcr=0.0831 centroid_hz=1248.0 flatness=0.2810",
        " rolloff_hz=2910.0 bands=0.0820,0.2310,0.3480,0.2290,0.1100",
        " category=normal confidence=0.884 label=paused raw_audio=disabled",
        "[NOTE] dBFS is relative digital level; it is not calibrated dB SPL",
        "[RESULT] PARTIAL_SIMULATED - I2S/features OK; SPL calibration absent",
    ],
}


def render(name: str, lines: list[str]) -> None:
    mono = font(25)
    small = font(20)
    line_height = 38
    width = 1900
    height = 90 + line_height * len(lines) + 70
    image = Image.new("RGB", (width, height), "#0b1220")
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((18, 18, width - 18, height - 18), 18,
                           fill="#111827", outline="#334155", width=2)
    draw.ellipse((42, 42, 62, 62), fill="#ef4444")
    draw.ellipse((72, 42, 92, 62), fill="#f59e0b")
    draw.ellipse((102, 42, 122, 62), fill="#22c55e")
    draw.text((148, 37), "VehicleSense · Serial Monitor", font=small,
              fill="#94a3b8")
    y = 92
    for line in lines:
        color = "#e2e8f0"
        if "[PASS]" in line or "connected" in line.lower() or "mounted" in line.lower():
            color = "#86efac"
        if "waiting" in line.lower() or "removed" in line.lower() or "[NOTE]" in line:
            color = "#fcd34d"
        draw.text((52, y), line, font=mono, fill=color)
        y += line_height
    image.save(OUT / name, optimize=True)


if __name__ == "__main__":
    for filename, content in CAPTURES.items():
        render(filename, content)
    print(f"Generadas {len(CAPTURES)} capturas en {OUT}")
