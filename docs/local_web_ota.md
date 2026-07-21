# Monitor local y OTA

`vehiclesense_wifi` crea un Access Point WPA2 y conserva simultáneamente la
conexión STA a Internet. El perfil celular heredado mantiene su AP. El monitor
queda normalmente en `http://192.168.4.1/`.

| Método | Ruta | Contenido |
|---|---|---|
| GET | `/` | Dashboard responsive |
| GET | `/api/status` | WiFi, SD/spool, MQTT, OTA, SIM heredado y uptime |
| GET | `/api/telemetry/basic` | Sensores, resumen IMU, audio y GPS textual |
| GET | `/admin` | Formulario OTA autenticado |
| POST | `/admin/update` | Carga multipart de `firmware.bin` |

La página muestra estado y coordenadas GPS como texto para diagnóstico local.
No incluye mapa, ruta ni recursos cartográficos externos.

El AP requiere WPA2. `/admin` y `/admin/update` añaden HTTP Basic con un usuario
y contraseña distintos. Si faltan credenciales, el AP o OTA se deshabilitan
sin detener sensores, microSD o SIM800L.

Los environments web, celular y VehicleSense usan `min_spiffs.csv`, con dos
particiones OTA. La primera carga se hace por USB. Para el perfil actual se
compila:

```bash
pio run -e vehiclesense_wifi
```

Se sube `.pio/build/vehiclesense_wifi/firmware.bin` desde `/admin`.
Durante la transferencia se pausan SD y MQTT. Solo se reinicia si
`Update.end(true)` confirma la imagen. Firma, HTTPS y rollback automático
quedan para una fase posterior.
