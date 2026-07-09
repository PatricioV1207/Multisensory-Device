# Bus Electric IoT Monitor — Prototype

## Overview

Firmware modular en PlatformIO para un prototipo de monitoreo multisensorial de
buses eléctricos universitarios. Lee un DHT11, un GPS NEO-6M y un GY-801,
valida cada fuente y publica telemetría JSON por MQTT sobre WiFi.

Los fallos son parciales: un sensor ausente no detiene el GPS, la red ni los
demás componentes. Las mediciones inválidas se omiten del JSON y se conservan
banderas de estado explícitas.

## Hardware de esta fase

- ESP32 DevKit V1 / NodeMCU-32 de 30 pines.
- DHT11.
- GPS NEO-6M.
- GY-801: ADXL345, L3G4200D, HMC5883L y BMP180/BMP085.
- WiFi integrado del ESP32.

No se incluyen módem celular, microSD, BME280, BH1750, micrófonos ni otros
módulos del sistema final.

## Arquitectura

`main.cpp` delega en `AppController`. Los sensores producen estructuras de
datos independientes; `TelemetryValidator` comprueba vigencia y rango;
`TelemetryBuilder` serializa únicamente valores válidos; y
`MQTTClientCustom` publica usando un transporte `Client` intercambiable.

Consulta [docs/architecture.md](docs/architecture.md) para el flujo completo.

## Instalación

1. Instala PlatformIO Core o la extensión PlatformIO para VS Code.
2. Abre esta carpeta como proyecto.
3. Compila el entorno principal:

```bash
pio run -e full_prototype
```

4. Conecta primero únicamente el GY-801 y ejecuta el scanner:

```bash
pio run -e test_i2c_scanner -t upload
pio device monitor -b 115200
```

La guía de conexiones está en [docs/wiring.md](docs/wiring.md).

## Configuración WiFi/MQTT

Copia `include/secrets_example.h` como `include/secrets.h` y reemplaza los
valores de ejemplo. El archivo local está excluido de Git.

ThingsBoard usa normalmente:

- Puerto: `1883`.
- Usuario: access token del dispositivo.
- Contraseña: vacía.
- Topic: `v1/devices/me/telemetry`.

Un broker MQTT convencional puede usar usuario, contraseña y tópico propios.
Esta fase utiliza MQTT sin TLS y debe considerarse una configuración de
laboratorio.

## Pruebas modulares

Los environments disponibles son:

```text
test_dht11         test_gps           test_i2c_scanner
test_adxl345       test_l3g4200d      test_hmc5883l
test_bmp180        test_gy801         test_wifi
test_mqtt_wifi     test_payload_json  full_prototype
```

Los comandos y criterios PASS/FAIL se encuentran en
[docs/tests.md](docs/tests.md).

## Estado actual

- Arquitectura y drivers implementados.
- Verificación por dirección e ID para los cuatro chips GY-801.
- Payload parcial, reconexión WiFi/MQTT y pruebas Unity implementados.
- Pendiente: validación con el hardware físico, calibración del magnetómetro y
  configuración de un dispositivo real en ThingsBoard.

## Roadmap

- MQTT con TLS.
- Sincronización NTP y timestamp Unix opcional.
- Cola persistente y logging en microSD.
- Transporte móvil SIM7600/SIM800L mediante otro `Client`.
- Sensores ambientales, luz y ruido adicionales.
- Calibración persistente y fusión de orientación roll/pitch/yaw.

