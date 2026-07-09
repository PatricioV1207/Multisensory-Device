# Cableado del prototipo

Usa los números GPIO, no etiquetas `Dxx`. Todos los módulos deben compartir
GND. Desconecta la alimentación antes de modificar el cableado.

| Módulo | Pin | ESP32 | Nota |
|---|---|---:|---|
| DHT11 | DATA | GPIO27 | Pull-up 4.7–10 kΩ a 3.3 V si el módulo no la incluye |
| DHT11 | VCC | 3V3 | Lógica de 3.3 V |
| DHT11 | GND | GND | Tierra común |
| NEO-6M | TX | GPIO16 (RX2) | Canal NMEA hacia el ESP32 |
| NEO-6M | RX | GPIO17 (TX2) | Opcional para configuración |
| NEO-6M | VCC | 3V3 o VIN válido | 5 V solo si el breakout declara regulador |
| NEO-6M | GND | GND | Tierra común |
| GY-801 | SDA | GPIO21 | Bus I2C a 3.3 V |
| GY-801 | SCL | GPIO22 | Bus I2C a 3.3 V |
| GY-801 | VCC | 3V3 | No elevar SDA/SCL a 5 V |
| GY-801 | GND | GND | Tierra común |

## Direcciones esperadas del GY-801

| Chip | Dirección | Identificador |
|---|---:|---|
| ADXL345 | `0x53` o `0x1D` | `DEVID = 0xE5` |
| L3G4200D | `0x69` o `0x68` | `WHO_AM_I = 0xD3` |
| HMC5883L | `0x1E` | ID ASCII `H43` |
| BMP180/BMP085 | `0x77` | Chip ID `0x55` |

Si aparece `0x0D` sin `0x1E`, el firmware lo reporta como posible QMC5883L y
no aplica el driver HMC5883L.

La primera prueba física debe ser `test_i2c_scanner`. No continúes con la
integración completa hasta documentar las direcciones observadas.

