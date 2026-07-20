# Bus Electric IoT Monitor — Prototype

## Overview

Firmware modular en PlatformIO para un prototipo de monitoreo multisensorial de
buses eléctricos universitarios. Lee DHT11, GPS NEO-6M, GY-801 y BH1750;
respalda telemetría en microSD y puede publicarla por MQTT sobre WiFi o por
GPRS mediante SIM800L. El perfil celular ofrece además un dashboard local y
actualización OTA desde el Access Point del ESP32.

Los fallos son parciales: un sensor ausente no detiene el GPS, la red ni los
demás componentes. Las mediciones inválidas se omiten del JSON y se conservan
banderas de estado explícitas.

## Hardware de esta fase

- ESP32 DevKit V1 / NodeMCU-32 de 30 pines.
- DHT11.
- GPS NEO-6M.
- GY-801: ADXL345, L3G4200D, HMC5883L y BMP180/BMP085.
- BH1750.
- Módulo microSD SPI.
- SIM800L V2 con fuente independiente adecuada.
- WiFi integrado del ESP32.

No se incluyen BME280, sensores de ruido, SIM7600 ni otros módulos del sistema
final. El frontend cloud completo tampoco forma parte de esta fase.

## Arquitectura

`main.cpp` delega en `AppController`. Los sensores producen estructuras de
datos independientes; `TelemetryValidator` comprueba vigencia y rango;
`TelemetryBuilder` serializa únicamente valores válidos; y
`MQTTClientCustom` publica usando un transporte `Client` intercambiable.
La referencia del BMP180 puede calibrarse con una altitud conocida o con un fix
GPS controlado y persistirse en NVS.

`full_prototype` conserva el transporte MQTT por WiFi. El environment
`full_prototype_cellular` añade BH1750, microSD, AP/web/OTA y transporte
SIM800L. En este último, cada payload se guarda primero en la tarjeta y se
publica después si GPRS, TLS y MQTT están disponibles.

Consulta [docs/architecture.md](docs/architecture.md) para el flujo completo.

## Instalación

1. Instala PlatformIO Core o la extensión PlatformIO para VS Code.
2. Abre esta carpeta como proyecto.
3. Compila el entorno WiFi o el perfil celular:

```bash
pio run -e full_prototype
pio run -e full_prototype_cellular
```

4. Conecta primero únicamente el GY-801 y ejecuta el scanner:

```bash
pio run -e test_i2c_scanner -t upload
pio device monitor -b 115200
```

La guía de conexiones está en [docs/wiring.md](docs/wiring.md).

## Configuración de red, AP y MQTT

Copia `include/secrets_example.h` como `include/secrets.h` y reemplaza los
valores de ejemplo. El archivo local está excluido de Git.

ThingsBoard usa normalmente:

- Puerto: `1883`.
- Usuario: access token del dispositivo.
- Contraseña: vacía.
- Topic: `v1/devices/me/telemetry`.

Un broker MQTT convencional puede usar usuario, contraseña y tópico propios.
El perfil WiFi existente utiliza MQTT sin TLS y debe considerarse una
configuración de laboratorio.

Para el perfil celular configura además APN, PIN opcional, broker TLS, Access
Point y credenciales del administrador. El MQTT celular real requiere TLS de
forma predeterminada; no degrada silenciosamente a texto plano si el SIM800L no
supera TLS/SNI. El environment `test_sim800l_mqtt` usa únicamente datos
sintéticos para validar TCP/1883.

Consulta [docs/cellular_mqtt.md](docs/cellular_mqtt.md),
[docs/storage.md](docs/storage.md) y
[docs/local_web_ota.md](docs/local_web_ota.md).

## Pruebas modulares

Los environments disponibles son:

```text
test_dht11         test_gps           test_i2c_scanner
test_adxl345       test_l3g4200d      test_hmc5883l
test_bmp180        calibrate_bmp180   test_gy801
test_wifi          test_mqtt_wifi     test_payload_json
test_barometer_math test_bh1750       test_microsd
test_sim800l_at     test_sim800l_gprs test_sim800l_mqtt
test_sim800l_mqtt_tls                 test_local_web
test_local_ota      test_local_web_json
full_prototype      full_prototype_cellular
```

Los comandos y criterios PASS/FAIL se encuentran en
[docs/tests.md](docs/tests.md).

## Estado actual

- Arquitectura y drivers implementados.
- Verificación por dirección e ID para los cuatro chips GY-801.
- Calibración persistente del BMP180 con altitud conocida o GPS.
- Payload v2 parcial, reconexión y pruebas Unity implementados.
- Respaldo JSONL, BH1750, estado celular, dashboard local y OTA implementados.
- Pendiente: validar físicamente cobertura 2G y TLS/SNI del firmware concreto
  del SIM800L, probar cortes de alimentación y configurar el broker cloud.

## Roadmap

- Sincronización NTP y timestamp Unix opcional.
- Reenvío controlado del backlog almacenado en microSD.
- Migración a SIM7600 si TLS/SNI del SIM800L no resulta viable.
- Firma criptográfica y rollback automático de firmware OTA.
- Frontend cloud con GPS, mapa, rutas, gráficas e historial según
  [docs/cloud_architecture.md](docs/cloud_architecture.md).
- Sensores ambientales y de ruido adicionales.
- Fusión de orientación roll/pitch/yaw.
