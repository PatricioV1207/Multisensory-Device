# VehicleSense — plataforma de monitoreo vehicular

## Overview

VehicleSense es una plataforma de monitoreo multisensorial para vehículos de
propósito general. El repositorio contiene hoy el firmware modular PlatformIO
del ESP32 y está evolucionando, por fases, hacia una solución integrada con
HiveMQ Cloud, backend, PostgreSQL y aplicación web de flota.

El firmware existente lee DHT11, GPS NEO-6M, GY-801 y BH1750; puede respaldar
telemetría en microSD y publicarla por MQTT sobre WiFi. También conserva un
perfil experimental SIM800L y una página local con OTA. La fase activa utilizará
WiFi; el trabajo celular se preserva pero queda diferido.

Los fallos son parciales: un sensor ausente no detiene el GPS, la red ni los
demás componentes. Las mediciones inválidas se omiten del JSON y se conservan
banderas de estado explícitas.

## Hardware actual y ampliación prevista

- ESP32 DevKit V1 / NodeMCU-32 de 30 pines.
- DHT11.
- GPS NEO-6M.
- GY-801: ADXL345, L3G4200D, HMC5883L y BMP180/BMP085.
- BH1750.
- Módulo microSD SPI.
- SIM800L V2 con fuente independiente adecuada.
- WiFi integrado del ESP32.
- INMP441 I2S integrado en firmware; validación física pendiente.

No se incluyen BME280, SIM7600, OBD-II ni mediciones de combustible o batería
del vehículo. Los valores no disponibles no se sustituyen por cero.

## Arquitectura

`main.cpp` delega en `AppController`. Los sensores producen estructuras de
datos independientes; `TelemetryValidator` comprueba vigencia y rango;
`TelemetryBuilder` serializa únicamente valores válidos; y
`MQTTClientCustom` publica usando un transporte `Client` intercambiable.
La referencia del BMP180 puede calibrarse con una altitud conocida o con un fix
GPS controlado y persistirse en NVS.

`full_prototype` conserva el transporte MQTT por WiFi. `vehiclesense_wifi` es
el perfil actual: AP+STA, BH1750, microSD, página/OTA, tiempo NTP/GPS y MQTT
QoS 1 sobre TLS hacia HiveMQ Cloud. Cada muestra queda en un spool SD acotado
hasta recibir PUBACK, además del archivo JSONL de auditoría. El environment
`full_prototype_cellular` añade BH1750, microSD, AP/web/OTA y transporte
SIM800L. En este último, cada payload se guarda primero en la tarjeta y se
publica después si GPRS, TLS y MQTT están disponibles.

La arquitectura objetivo mantiene el firmware en la raíz y añade componentes
separados en `backend/`, `frontend/`, `simulator/` y `deploy/`. Los contratos
versionados viven en [`contracts/`](contracts/README.md). La auditoría y la hoja
de ruta completa están en
[`docs/vehiclesense_implementation_plan.md`](docs/vehiclesense_implementation_plan.md).

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

## Configuración de red, AP y MQTT actual

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

El perfil `vehiclesense_wifi` usa HiveMQ Cloud por MQTT/TLS 8883 con ACL por
dispositivo, LWT, estado retenido y QoS 1. La estructura está especificada en
[`contracts/mqtt-topics.md`](contracts/mqtt-topics.md).

Para el perfil celular configura además APN, PIN opcional, broker TLS, Access
Point y credenciales del administrador. El MQTT celular real requiere TLS de
forma predeterminada; no degrada silenciosamente a texto plano si el SIM800L no
supera TLS/SNI. El environment `test_sim800l_mqtt` usa únicamente datos
sintéticos para validar TCP/1883.

Consulta [docs/cellular_mqtt.md](docs/cellular_mqtt.md),
[docs/storage.md](docs/storage.md) y
[docs/local_web_ota.md](docs/local_web_ota.md). El alcance, privacidad y
limitaciones del INMP441 están en
[docs/acoustic_monitoring.md](docs/acoustic_monitoring.md).

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
test_local_ota      test_local_web_json test_mqtt_topics
test_offline_queue  full_prototype      full_prototype_cellular
vehiclesense_wifi
test_inmp441       collect_acoustic_features test_acoustic_classifier
```

Los comandos y criterios PASS/FAIL se encuentran en
[docs/tests.md](docs/tests.md).

## Estado actual

- Arquitectura y drivers implementados.
- Verificación por dirección e ID para los cuatro chips GY-801.
- Calibración persistente del BMP180 con altitud conocida o GPS.
- Payload v2 parcial, reconexión y pruebas Unity implementados.
- Respaldo JSONL, spool offline acotado, BH1750, estado celular conservado,
  dashboard local y OTA implementados.
- Auditoría VehicleSense completada: los 21 environments ESP32 compilan y las
  13 pruebas nativas existentes pasan.
- Contratos MQTT VehicleSense v1 y telemetría v3 definidos, manteniendo un
  schema explícito para telemetría v2.
- Perfil `vehiclesense_wifi` integrado con AP+STA, NTP/GPS, HiveMQ/TLS, QoS 1,
  LWT, tópicos por dispositivo y PUBACK.
- INMP441 integrado con nivel dBFS relativo, FFT, características, categorías
  heurísticas, alertas sostenidas y colección etiquetada sin audio crudo.
- Pendiente: validación física de INMP441 y de credenciales HiveMQ/ACL, backend,
  PostgreSQL, frontend, simulador y despliegue.

## Roadmap

- Validación física de `vehiclesense_wifi` contra HiveMQ Cloud.
- Validación y dataset físico del INMP441; calibración SPL solo si se incorpora
  una referencia acústica adecuada.
- Backend FastAPI, PostgreSQL, REST/WebSocket, alertas y viajes.
- Frontend VehicleSense con dashboard, mapa, detalle e históricos.
- Simulador multivehículo y despliegue Docker/Nginx en Oracle Cloud.
- Fase celular posterior; SIM800L permanece conservado.
