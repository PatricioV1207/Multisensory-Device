# Estado actual del proyecto

Fecha de corte: **2026-08-31**. Este estado combina lo observable en el repositorio
con comprobaciones públicas fechadas; no acredita hardware físico ni controles
operativos que no hayan sido observados.

## Línea base y evidencia

- Base auditada: rama `main`, commit `510f27c` más las correcciones locales de esta
  validación de despliegue.
- La auditoría estática contrastó `platformio.ini`, firmware, contratos JSON/MQTT,
  migraciones, rutas FastAPI, cliente React, simulador, Compose/Nginx y scripts.
- Al cierre de esta reorganización, el `rg` de documentos/wrappers retirados devolvió
  0 coincidencias, `git diff --check` no reportó errores y la validación `bash -n`
  aceptó los 6 scripts restantes de `deploy/scripts/`.
- La matriz automatizada se ejecutó el 2026-08-30: 24/24 builds ESP32, 34/34 casos
  Unity nativos, contratos con 9 fixtures válidos aceptados y 7 inválidos
  rechazados, backend Ruff/formato/pytest 15/15, frontend ESLint/Prettier/Vitest
  6/6/build con `tsc -b`, y simulador Ruff/formato/pytest 10/10.
- Alembic pasó `upgrade head → downgrade base → upgrade head` sobre SQLite y la
  generación offline para PostgreSQL produjo 330 líneas. No hubo conexión a una
  instancia PostgreSQL real.
- También pasaron la validación estática de Compose, `bash -n` de scripts y sintaxis
  JSON. Esto no ejecutó contenedores ni servicios externos.
- El 2026-08-31 se construyeron las imágenes backend/frontend y se levantó el Compose
  productivo en Docker Desktop ARM64 con PostgreSQL local: Alembic llegó a `head`,
  Nginx y los cuatro servicios quedaron saludables, la página y el login funcionaron,
  y backup/checksum/restore finalizaron correctamente. La plantilla HTTPS pasó
  `nginx -t` con un certificado local efímero; no se solicitó un certificado real.
- ShellCheck aceptó los scripts después de corregir dos asignaciones, Hadolint no
  dejó advertencias en los Dockerfiles y las auditorías `npm audit`/`pip-audit` no
  reportaron vulnerabilidades conocidas después de actualizar los locks.
- La primera invocación global `python3 contracts/validate_fixtures.py` falló porque
  ese intérprete no tenía `jsonschema`. La ejecución
  `backend/.venv/bin/python contracts/validate_fixtures.py` pasó; fue un requisito
  de entorno, no un fallo de los contratos.
- El repositorio no contiene inventario de la cuenta AWS, secretos, logs privados ni
  evidencia física de la placa. El contexto de despliegue aportado por el usuario
  reporta EC2, DNS, Let's Encrypt, cron y HiveMQ configurados; esos datos no deben
  convertirse en secretos versionados.
- La comprobación pública del 2026-08-31 confirmó que ambos dominios resuelven a la
  Elastic IP, HTTP redirige a HTTPS, la página responde 200, `/health/live` responde
  200 y `/health/ready` responde 200 con PostgreSQL y MQTT conectados. Esto sustituye
  la observación previa de GitHub Pages; no valida swap, cron, backups externos ni
  sensores físicos.
- La interfaz y la documentación pública no usan un nombre de producto. Los
  identificadores técnicos heredados de perfiles, tópicos, paquetes y variables se
  conservan únicamente para no romper contratos ni despliegues existentes.
- El retiro de la denominación pública se validó el 2026-08-31 con búsqueda textual
  sin coincidencias del nombre, build del perfil integrado, lint/pruebas/build del
  frontend, Ruff/pytest del backend y simulador, fixtures contractuales y sintaxis
  de los scripts de despliegue modificados.

### Observaciones históricas no reproducibles

Un registro manual retirado durante la consolidación afirmaba que DHT compiló y se
cargó, con lecturas estables pero sin comprobar exactitud; que el scanner I2C observó
las cuatro direcciones del GY-801 y emitió
`[W][Wire.cpp:301] begin(): Bus already started in Master Mode`; y que ADXL345
requería mejorar la calibración. No tenía fecha, logs ni artefactos asociados. Estas
observaciones **no son evidencia vigente** y deben repetirse antes de aceptar el
hardware.

## Estado por área

| Área | Estado | Evidencia presente en el repositorio | Parcial, ausente o no demostrado |
|---|---|---|---|
| Firmware | Parcial | Perfiles PlatformIO, sensores, validación, WiFi MQTT/TLS QoS 1, LWT, web local, OTA, microSD y acústica | Todo comando válido recibe ACK `unsupported`/`COMMAND_HANDLER_DEFERRED`; OTA sin firma ni rollback; hardware no validado |
| Contratos | Implementado en repositorio | Telemetría v2/v3 y contratos v1 para status, eventos, acústica, comandos/acuses; tópicos `vehiclesense/v1` y fixtures | Compatibilidad real entre clientes externos no demostrada; los contratos no garantizan entrega física |
| Backend | Parcial | Auth y roles, REST `/api/v1`, WS `/ws/v1/live`, MQTT, validación/dedupe/cuarentena, alertas, viajes inferidos por GPS y publicación de comandos | Refresh sin persistencia/revocación/logout; sin ACL por vehículo, API de historial de estado, tabla genérica de reglas, cola interna acotada, cleanup de retención ni E2E PostgreSQL/broker real |
| Frontend | Parcial funcional | REST/WS, modo demo explícito, flota, mapas, telemetría, acústica, alertas, viajes, analítica y exportación | Settings es informativo; no aplica ACL visual por rol/vehículo; exporta en cliente solo las últimas 24 h; pruebas orientadas a demo; varias vistas confunden no simulado con producción |
| Simulador | Implementado en repositorio | Payloads contractuales, TLS/QoS 1, escenarios, errores explícitos y replay acotado en memoria | ACK de comandos simula respuesta, no ejecución semántica; sin evidencia de broker real; cola no durable |
| Despliegue | Producción reportada y endpoints públicos validados | Compose, imágenes, Nginx HTTP/HTTPS, migraciones, login y backup/restore validados localmente; dominios públicos, TLS, `/health/live`, `/health/ready` y conexión MQTT observados el 2026-08-31 | Instancia única sin HA; swap, cron, backups externos, monitorización, ACL cruzadas y sesión física de ESP32 requieren comprobación en EC2; OCI es alternativa histórica |
| Hardware/PCB | No validado | Pinout y especificación técnica Rev A en documentación | La PCB sigue en borrador/no fabricar; sin esquema/PCB/gerbers/BOM final ni evidencia de bring-up; SIM800L y clasificador acústico experimentales |

## Garantías y problemas conocidos

- Solo telemetría v3 usa spool durable en microSD y replay tras reconexión. Acústica
  y eventos se publican en vivo y se archivan en JSONL, pero no tienen replay durable.
- `/api/telemetry/basic` del ESP32 incluye GPS textual; `/api/status` omite GPS.
- WebSocket entrega cambios en vivo, no historial; al reconectar se debe refrescar REST.
- `TripsPage`, `DevicesPage` y `VehicleDetailPage` muestran “Producción” cuando
  `simulated=false`; ese flag solo significa no simulado. La etiqueta correcta debe
  ser “Real/no simulado” hasta disponer de procedencia productiva verificable.
- Los viajes se infieren por GPS. No existen ignición, OBD, combustible ni diagnóstico
  certificado.
- dBFS es relativo y el clasificador `heuristic-1` no tiene precisión validada.
- SIM800L depende de 2G, alimentación y TLS del entorno; no es ruta aceptada de
  producción.
- No hay política automatizada de retención ni prueba de restore/operación real.

## Próxima tarea recomendada

Abrir un chat nuevo para **completar la línea base E2E con IDs reales de vehículo y
dispositivo**, simulador/ESP32, deduplicación, cuarentena, REST/WS, roles, backup,
monitorización y costos. No reprovisionar EC2, DNS, TLS ni HiveMQ; registrar por
separado lo observado en hardware y lo observado en cloud.
