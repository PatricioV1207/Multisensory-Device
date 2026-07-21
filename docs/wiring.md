# Cableado del prototipo

Usa los números GPIO, no etiquetas `Dxx`. Todos los módulos deben compartir
GND. Desconecta la alimentación antes de modificar el cableado.

| Módulo | Pin | ESP32 | Nota |
|---|---|---:|---|
| DHT11 | DATA | GPIO27 | Pull-up 4.7–10 kΩ a 3.3 V si el módulo no la incluye |
| DHT11 | VCC | 3V3 | Lógica de 3.3 V |
| DHT11 | GND | GND | Tierra común |
| NEO-6M | TX | GPIO32 (RX2) | Canal NMEA hacia el ESP32 |
| NEO-6M | RX | GPIO33 (TX2) | Opcional para configuración |
| NEO-6M | VCC | 3V3 o VIN válido | 5 V solo si el breakout declara regulador |
| NEO-6M | GND | GND | Tierra común |
| GY-801 | SDA | GPIO21 | Bus I2C a 3.3 V |
| GY-801 | SCL | GPIO22 | Bus I2C a 3.3 V |
| GY-801 | VCC | 3V3 | No elevar SDA/SCL a 5 V |
| GY-801 | GND | GND | Tierra común |
| BH1750 | SDA | GPIO21 | I2C compartido, `0x23` o `0x5C` |
| BH1750 | SCL | GPIO22 | I2C compartido |
| BH1750 | VCC | 3V3 | Lógica a 3.3 V |
| BH1750 | GND | GND | Tierra común |
| microSD | SCK | GPIO18 | SPI |
| microSD | MISO | GPIO19 | SPI |
| microSD | MOSI | GPIO23 | SPI |
| microSD | CS | GPIO5 | Pull-up recomendado por ser pin de arranque |
| SIM800L | TX | GPIO16 (RX1) | Módem hacia ESP32 |
| SIM800L | RX | GPIO17 (TX1) | Verificar niveles del breakout V2 |
| INMP441 | SCK/BCLK | GPIO26 | Reloj I2S generado por el ESP32 |
| INMP441 | WS/LRCL | GPIO25 | Selección de palabra I2S |
| INMP441 | SD | GPIO34 | Datos I2S hacia el ESP32; GPIO34 es solo entrada |
| INMP441 | L/R | GND | Selecciona el canal izquierdo usado por el firmware |
| INMP441 | VDD | 3V3 | No alimentar con 5 V |
| INMP441 | GND | GND | Tierra común y cableado corto |

## Direcciones I2C esperadas

| Chip | Dirección | Identificador |
|---|---:|---|
| ADXL345 | `0x53` o `0x1D` | `DEVID = 0xE5` |
| L3G4200D | `0x69` o `0x68` | `WHO_AM_I = 0xD3` |
| HMC5883L | `0x1E` | ID ASCII `H43` |
| BMP180/BMP085 | `0x77` | Chip ID `0x55` |
| BH1750 | `0x23` o `0x5C` | Dirección según pin ADD |

Si aparece `0x0D` sin `0x1E`, el firmware lo reporta como posible QMC5883L y
no aplica el driver HMC5883L.

La primera prueba física debe ser `test_i2c_scanner`. No continúes con la
integración completa hasta documentar las direcciones observadas.

SIM800L debe usar fuente independiente capaz de entregar picos de al menos 2 A,
condensador de 470–1000 µF cerca del módulo y GND común. No alimentarlo desde
3V3 del ESP32.

## INMP441

El INMP441 se conecta por I2S y no comparte el bus I2C. Mantén BCLK, WS y SD
lo más cortos posible y evita pasarlos junto a la alimentación pulsante del
SIM800L. La selección `L/R = GND` es obligatoria con la configuración actual,
que captura únicamente el canal izquierdo. Si el módulo se configura como
canal derecho, el diagnóstico mostrará señal ausente o inválida.

Antes de integrar el micrófono al vehículo, ejecuta `test_inmp441` y confirma
físicamente que voz, palmadas y silencio cambian el nivel y las características.
Que el firmware compile no prueba el pinout ni la integridad de la señal I2S.
