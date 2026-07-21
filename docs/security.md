# Seguridad y límites de confianza

## Fronteras

```text
ESP32 ── credencial de dispositivo ──► HiveMQ Cloud
Backend ─ credencial de backend ─────► HiveMQ Cloud
Navegador ─ JWT HTTPS/WSS ───────────► Nginx/FastAPI
FastAPI ─ red Docker interna ────────► PostgreSQL
```

El frontend no contiene credenciales MQTT o de base. HiveMQ valida conexión y
ACL; el backend vuelve a validar tópico, identidad, tamaño, JSON Schema y
finitud antes de persistir. PostgreSQL no publica puerto. El sistema sigue
registrando localmente si Internet falla.

## Controles implementados

- MQTT/TLS con CA y hostname, client ID único, LWT, QoS 1 y ACL por identidad.
- Credenciales en archivos/variables ignorados por Git.
- JWT access/refresh, Argon2 y roles `admin`, `operator`, `viewer`.
- WebSocket autenticado en el primer frame; el token no viaja en la URL.
- Nginx HTTPS, HSTS, CSP, `nosniff`, bloqueo de frames y rate limit de login.
- Payload estricto, máximo de tamaño, cuarentena e ingestión idempotente.
- Backend y Nginx con filesystem read-only donde es práctico y
  `no-new-privileges` en Compose.
- Solo Nginx publica 80/443; PostgreSQL se conecta por red interna.
- Backups con checksum y permisos restrictivos; restore requiere confirmación.
- Audio crudo deshabilitado; solo se almacenan agregados/características.

## Secretos

No versionar:

- `include/secrets.h`;
- `backend/.env`, `simulator/.env` o `deploy/.env`;
- certificados/llaves Certbot;
- dumps de PostgreSQL;
- tokens JWT o credenciales en capturas y registros.

Use credenciales distintas para cada ESP32, backend y simulador. Rote una
credencial comprometida sin reciclarla. El password HiveMQ del ESP32 queda
compilado en flash; un atacante con acceso físico y técnicas de extracción
puede obtenerlo, por lo que su ACL debe limitar el daño a una sola identidad.

## Web y sesiones

El access y refresh token se guardan en `sessionStorage`, no en cookies. Esto
reduce persistencia entre sesiones pero no protege contra XSS; por eso no se
permiten scripts externos y Nginx aplica CSP. Los roles se verifican en FastAPI,
no solo ocultando controles en React.

El WebSocket exige `{action: "authenticate", token: "..."}` en los primeros
cinco segundos. El proxy deshabilita access logs en `/ws/`. Tras reconectar, el
cliente vuelve a consultar REST porque el canal no es historial.

## OTA local

La OTA del ESP32 está detrás de WPA2 y HTTP Basic, y solo debe exponerse en el
AP/red local. No tiene HTTPS, firma de imagen ni rollback automático. No
publique el puerto del AP hacia Internet y conserve USB como recuperación. Una
fase futura debe añadir firma criptográfica si el riesgo operativo lo exige.

## Datos y privacidad

GPS revela ubicación e historial de movimiento. Limite acceso por rol, defina
una política de retención y proteja backups. VehicleSense no implementa aún
borrado programado o anonimización.

El INMP441 no hace reconocimiento de voz ni identificación de hablantes. Audio
PCM está deshabilitado; habilitarlo en el futuro exige consentimiento, finalidad
documentada, retención mínima y revisión legal. dBFS no equivale a dB SPL.

## Riesgos pendientes

- No hay MFA, revocación central inmediata de JWT ni gestor externo de secretos.
- La topología OCI propuesta es una sola VM y no tiene alta disponibilidad.
- Las copias externas y el cifrado/retención fuera de la VM son operativos, no
  automáticos.
- El frontend usa tiles públicos OpenStreetMap y debe respetar su política.
- El clasificador acústico carece de dataset real y métricas de precisión.
- La detección de viaje usa GPS; no existe señal de ignición ni OBD.
- SIM800L/TLS se conserva como experimento y no forma parte de producción.

Antes de producción realice revisión de dependencias, escaneo de imágenes,
prueba de restauración, rotación de credenciales, prueba de penetración básica
y ejercicio de pérdida de HiveMQ/Internet.
