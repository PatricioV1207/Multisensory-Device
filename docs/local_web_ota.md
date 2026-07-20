# Monitor local y OTA

`full_prototype_cellular` crea un Access Point WPA2 con las credenciales de
`secrets.h`. El monitor queda normalmente en `http://192.168.4.1/`.

| Método | Ruta | Contenido |
|---|---|---|
| GET | `/` | Dashboard responsive |
| GET | `/api/status` | SD, SIM800L, GPRS, MQTT, OTA y uptime |
| GET | `/api/telemetry/basic` | DHT11, BH1750, BMP180 y resumen IMU |
| GET | `/admin` | Formulario OTA autenticado |
| POST | `/admin/update` | Carga multipart de `firmware.bin` |

Los tipos usados por la web local no contienen datos GPS. Ninguna respuesta
local expone coordenadas, velocidad, satélites ni altitud GPS.

El AP requiere WPA2. `/admin` y `/admin/update` añaden HTTP Basic con un usuario
y contraseña distintos. Si faltan credenciales, el AP o OTA se deshabilitan
sin detener sensores, microSD o SIM800L.

Los environments web y celular usan `min_spiffs.csv`, con dos particiones OTA.
La primera carga se hace por USB. Después se compila:

```bash
pio run -e full_prototype_cellular
```

Se sube `.pio/build/full_prototype_cellular/firmware.bin` desde `/admin`.
Durante la transferencia se pausan SD y MQTT. Solo se reinicia si
`Update.end(true)` confirma la imagen. Firma, HTTPS y rollback automático
quedan para una fase posterior.
