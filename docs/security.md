# Seguridad y límites de confianza

## Fronteras

```text
ESP32 ── credencial de dispositivo ──► HiveMQ Cloud
Backend ─ credencial de backend ─────► HiveMQ Cloud
Navegador ─ JWT HTTPS/WSS ───────────► Nginx/FastAPI
FastAPI ─ red Docker interna ────────► PostgreSQL
```

El frontend no contiene credenciales MQTT o de base. El backend valida tópico,
identidad, tamaño, JSON Schema y finitud antes de persistir. HiveMQ es una frontera
externa que debe autenticar y aplicar ACL, pero el repositorio no contiene la
configuración exportada ni evidencia de un cluster real que demuestre su aplicación.
PostgreSQL no publica puerto en Compose. El firmware sigue registrando localmente si
Internet falla.

## Controles implementados en código o configuración local

- Clientes MQTT/TLS con validación de CA y hostname, client ID configurable, LWT y
  QoS 1.
- Credenciales en archivos/variables ignorados por Git.
- JWT access/refresh autocontenidos, Argon2 y roles `admin`, `operator`, `viewer`.
- WebSocket autenticado en el primer frame; el token no viaja en la URL.
- Nginx HTTPS, HSTS, CSP, `nosniff`, bloqueo de frames y rate limit de login.
- Payload estricto, máximo de tamaño, cuarentena e ingestión idempotente.
- Backend y Nginx con filesystem read-only donde es práctico y
  `no-new-privileges` en Compose.
- Solo Nginx publica 80/443; PostgreSQL se conecta por red interna.
- Backups con checksum y permisos restrictivos; restore requiere confirmación.
- Audio crudo deshabilitado; solo se almacenan agregados/características.

Estos controles describen artefactos versionados; no prueban que Nginx, PostgreSQL,
HiveMQ o una VM estén desplegados con esa configuración.

## Controles externos requeridos y no verificados

- Crear credenciales distintas para cada dispositivo, backend y simulador.
- Aplicar en HiveMQ las ACL documentadas por identidad: cada dispositivo limitado a
  sus tópicos y el backend limitado a ingesta y publicación de comandos.
- Verificar TLS, ACL, revocación de credenciales y rechazo de accesos cruzados contra
  un cluster real antes de aceptar un despliegue.

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
no solo ocultando controles en React. No existe todavía ACL por vehículo: cada
usuario autenticado puede consultar toda la flota. Los refresh tokens no se
persisten ni se pueden revocar individualmente y no hay endpoint logout; cerrar
sesión en React solo elimina tokens locales.

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

- No hay MFA, sesiones refresh persistidas/revocables, logout ni gestor externo de
  secretos.
- No hay ACL por vehículo; los roles actuales operan sobre toda la flota.
- Las ACL MQTT están documentadas como requisito, pero no verificadas en HiveMQ real.
- La topología productiva AWS usa una sola instancia EC2 y no tiene alta
  disponibilidad; OCI se conserva solo como alternativa histórica.
- Las copias externas y el cifrado/retención fuera de la VM son operativos, no
  automáticos.
- El frontend usa tiles públicos OpenStreetMap y debe respetar su política.
- El clasificador acústico carece de dataset real y métricas de precisión.
- La detección de viaje usa GPS; no existe señal de ignición ni OBD.
- SIM800L/TLS se conserva como experimento y no forma parte de producción.
- Los comandos publicados por backend no se ejecutan en el firmware: se acusan como
  `unsupported`. No deben describirse como control remoto disponible.

Antes de producción realice revisión de dependencias, escaneo de imágenes,
prueba de restauración, rotación de credenciales, prueba de penetración básica
y ejercicio de pérdida de HiveMQ/Internet.
