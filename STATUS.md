# Estado actual del proyecto

Fecha de corte: **2026-09-02**. Este estado combina lo observable en el repositorio
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
- El repositorio incorpora un preflight de solo lectura para futuras sesiones E2E.
  Una auditoría SSH equivalente del 2026-08-31 confirmó swap de 2 GiB activo y
  persistente, disco raíz al 25 %, `deploy/.env` con modo 600, auto-registro apagado,
  variables bootstrap activas vacías, cron activo con una renovación a las 03:17 UTC,
  los cuatro servicios saludables, `unattended-upgrades` activo y readiness con
  PostgreSQL/MQTT conectados. El log de renovación aún no existe y la caché APT
  enumera 116 paquetes actualizables.
- PostgreSQL productivo tiene 1 vehículo (`vehicle-001`) y 1 dispositivo físico
  (`esp32-d0ef7645f640`); solo existe un usuario `admin`.
  Ambos IDs se registraron mediante la API con auto-registro apagado. El 2026-09-01
  UTC se generó un backup
  lógico local con checksum válido y se restauró en una base temporal aislada con la
  revisión Alembic esperada; no existe evidencia de copia cifrada fuera de EC2. Tres
  copias no rastreadas `deploy/.env.save*` que duplicaban secretos y conservaban las
  variables bootstrap pobladas se eliminaron con autorización del usuario. El
  `deploy/.env` activo permaneció con modo 600, bootstrap vacío y el worktree remoto
  quedó limpio; la necesidad de rotar credenciales aún debe evaluarse sin publicar
  sus valores.
- La auditoría AWS de solo lectura del 2026-08-31 observó una instancia `t4g.small`
  en ejecución con 3/3 comprobaciones aprobadas e IMDSv2 obligatorio. El monitoreo
  detallado está desactivado, no existen alarmas CloudWatch, el volumen raíz EBS de
  30 GiB no está cifrado y no hay snapshots EBS ni planes de AWS Backup en la región.
  Existe un presupuesto mensual excedido con tres alertas y un destinatario por
  alerta. El acceso SSH se validó desde la red autorizada sin modificar el Security
  Group.
- La sesión física del 2026-08-31 identificó por eFuse un ESP32-D0WD-V3 revisión
  3.1 con MAC `d0:ef:76:45:f6:40`. En HiveMQ Cloud se creó una credencial exclusiva
  con permiso limitado a
  `vehiclesense/v1/vehicles/vehicle-001/devices/esp32-d0ef7645f640/#`; la contraseña
  permanece solo en `include/secrets.h`, ignorado por Git. El perfil
  `vehiclesense_wifi` compiló y se cargó por USB. La sesión detectó y corrigió dos
  defectos: el límite superior de fecha desbordaba `time_t` en ESP32 y el bundle CA
  de Arduino se adjuntaba sin inicializar. Con comparación de epoch en 64 bits y la
  CA pública ISRG Root X1 explícita, el hotspot se asoció, NTP quedó válido, TLS
  verificó hostname y MQTT recibió PUBACK QoS 1. La API productiva observó el
  dispositivo `online`, firmware `0.2.0`, `simulated=false`, y devolvió 20 muestras
  físicas recientes; la última consultada fue
  `esp32-d0ef7645f640:13:275` a las 03:55:26 UTC del 2026-09-01.
- La interfaz y la documentación pública no usan un nombre de producto. Los
  identificadores técnicos heredados de perfiles, tópicos, paquetes y variables se
  conservan únicamente para no romper contratos ni despliegues existentes.
- El 2026-09-02 se conectó en la web el detalle ya persistido de viajes y
  `trip_points`: el historial usa una ventana semanal por defecto (con filtros de 1,
  7 y 30 días), cada tarjeta abre el mapa del recorrido completo y muestra métricas,
  origen/destino, cobertura y HDOP medio. La prueba de API validó persistencia y
  consulta de cuatro puntos GPS; otra prueba validó la reconstrucción cronológica de
  un viaje y cuatro puntos desde telemetría `replayed`. Esto no constituye evidencia
  de un nuevo viaje físico ni de un replay observado en hardware real.
- El despliegue productivo del commit `77d3f75` finalizó el 2026-09-03 UTC con los
  cuatro contenedores saludables, Alembic en `cd5ddc949e1d (head)`, `/health/live` y
  `/health/ready` en 200 y MQTT conectado. Antes de actualizar se creó el backup
  local `vehiclesense_20260903T015022Z.dump`, cuyo checksum se validó. La base
  productiva contenía un viaje completado y sus 47 puntos declarados coincidían con
  47 filas de ruta; esto acredita persistencia en PostgreSQL, no un nuevo recorrido
  físico posterior al despliegue ni una copia externa del backup.
- El 2026-09-03 UTC producción avanzó de `77d3f75` a `335ccc0`. El frontend
  desplegado conserva las mediciones originales y superpone tendencias visuales
  acotadas; usa media móvil de cinco muestras para temperatura, humedad y presión,
  mediana de tres para velocidad, preserva huecos inválidos y presenta las
  comparaciones entre vehículos como barras. La actualización creó el backup local
  `vehiclesense_20260903T022355Z.dump` con checksum válido y guardó la revisión
  previa. Los cuatro servicios quedaron saludables, Alembic permaneció en
  `cd5ddc949e1d (head)`, la auditoría operativa terminó con 0 fallos y 0 advertencias,
  `/health/live` y `/health/ready` respondieron 200 con PostgreSQL y MQTT listos, y
  `www` redirigió al dominio raíz. Esto valida build, despliegue y salud operativa;
  la inspección visual autenticada se realizó localmente, no sobre producción.
- El 2026-08-31 se incorporó evidencia visual del diseño, fabricación y montaje de
  la revisión física: PCB portadora de cuatro capas y caja impresa en PETG. Esta
  evidencia respalda el proceso documentado en el informe final, pero no sustituye
  el CAD fuente, el *stack-up*/Gerber versionado ni el *bring-up* funcional de los
  sensores.
- El retiro de la denominación pública se validó el 2026-08-31 con búsqueda textual
  sin coincidencias del nombre, build del perfil integrado, lint/pruebas/build del
  frontend, Ruff/pytest del backend y simulador, fixtures contractuales y sintaxis
  de los scripts de despliegue modificados.
- El 2026-09-03 se añadió `diagnose_inmp441_raw`, un perfil aislado que lee ambos
  slots I2S y reporta estadísticas de palabras crudas. Una ejecución física
  aportada por el usuario mostró `driver=1`, aproximadamente 250 lecturas por
  intervalo, cero fallos, `slot0` con 32 000 palabras casi todas no nulas y
  variables, y `slot1` completamente en cero. Esto valida actividad digital desde
  el INMP441 hasta el receptor I2S, pero no la exactitud acústica ni la selección
  mono del perfil integrado. Una variante provisional con el selector opuesto
  permitió comprobar el slot activo sin cambiar inicialmente `vehiclesense_wifi`.
  La prueba controlada posterior produjo `mic=1` y `analysis=1`: el silencio quedó aproximadamente entre
  -89.9 y -89.3 dBFS; un tono de 1 kHz subió a entre -80.6 y -80.0 dBFS y concentró
  entre 82.6 % y 84.1 % de energía en la banda 800--2000 Hz; las palmadas elevaron
  el pico hasta aproximadamente -51 dBFS. Esto valida respuesta relativa y
  discriminación espectral aproximada, no dB SPL ni precisión del clasificador.
  `test_inmp441`, `collect_acoustic_features` y `vehiclesense_wifi` usan ahora el
  selector mono del driver validado físicamente; `L/R` permanece cableado a GND.
  La calibración provisional `heuristic-4` fija silencio en -88 dBFS y reconoce
  como `horn`, con confianza inferior al umbral de alerta, los patrones aportados
  para un claxon posterior y el claxon del propio vehículo. Las pruebas de
  regresión no sustituyen un dataset balanceado ni miden falsos positivos. Una
  captura posterior en un cuarto con impresora 3D activa y conversación
  ocasional observó aproximadamente -89.6 a -65.8 dBFS, centroide de 2.6 a
  3.7 kHz y flatness de 0.36 a 0.50. `heuristic-4` clasifica ese fondo como
  `noise`, reserva `traffic` para señales superiores a -45 dBFS y no permite
  que `noise` genere alertas. Esto reduce el falso positivo observado, pero no
  mide precisión frente a tráfico vehicular real. `unknown` queda para señal
  inválida, detenida, no finita o con clipping. La comprobación física posterior
  observó `noise` tanto en el colector como en el payload de `vehiclesense_wifi`;
  las pruebas pasaron 9/9 casos acústicos, 11/11 casos de payload/validación,
  6/6 casos del validador backend y 9 fixtures válidos/7 inválidos. Ambos perfiles
  ESP32 compilaron y se cargaron por USB. Durante la última lectura el punto WiFi
  no estuvo disponible, por lo que no se validó la aceptación de `noise` en la
  instancia productiva.

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
| Firmware | Parcial con transporte físico validado | Perfiles PlatformIO, sensores, validación, WiFi MQTT/TLS QoS 1, LWT, web local, OTA, microSD y acústica; NTP, TLS, MQTT y PUBACK observados en el ESP32 real | Todo comando válido recibe ACK `unsupported`/`COMMAND_HANDLER_DEFERRED`; OTA sin firma ni rollback; sensores, replay y fallos inducidos aún no validados |
| Contratos | Implementado en repositorio | Telemetría v2/v3 y contratos v1 para status, eventos, acústica, comandos/acuses; tópicos `vehiclesense/v1` y fixtures | Compatibilidad real entre clientes externos no demostrada; los contratos no garantizan entrega física |
| Backend | Parcial con ingesta física validada | Auth y roles, REST `/api/v1`, WS `/ws/v1/live`, MQTT, validación/dedupe/cuarentena, alertas, viajes inferidos por GPS y publicación de comandos; estado `online` y muestras del ESP32 real observados en producción | Refresh sin persistencia/revocación/logout; sin ACL por vehículo, API de historial de estado, tabla genérica de reglas, cola interna acotada, cleanup de retención ni pruebas de replay/fallos físicos |
| Frontend | Parcial funcional | REST/WS, modo demo explícito, flota, mapas, telemetría, acústica, alertas, viajes, analítica y exportación | Settings es informativo; no aplica ACL visual por rol/vehículo; exporta en cliente solo las últimas 24 h; pruebas orientadas a demo; varias vistas confunden no simulado con producción |
| Simulador | Implementado en repositorio | Payloads contractuales, TLS/QoS 1, escenarios, errores explícitos y replay acotado en memoria | ACK de comandos simula respuesta, no ejecución semántica; sin evidencia de broker real; cola no durable |
| Despliegue | Producción y tramo físico básico validados | Compose, imágenes, Nginx HTTP/HTTPS, migraciones y login validados; dominios públicos, TLS, readiness, MQTT, swap, cron, backup/restore aislado e ingesta del ESP32 observados el 2026-08-31/09-01 UTC | Instancia única sin HA; backup solo local, volumen EBS sin cifrar, sin alarmas CloudWatch; faltan copia externa, rotación, ACL cruzadas y pruebas físicas de fallos/replay; OCI es alternativa histórica |
| Hardware/PCB | Diseño, fabricación y montaje visual documentados; sensores no validados | ESP32-D0WD-V3 rev. 3.1, MAC eFuse y carga serial observados el 2026-08-31; payload local con IDs reales; evidencia visual de PCB portadora de cuatro capas y caja PETG | El CAD fuente, *stack-up*/Gerber/BOM y *bring-up* funcional aún no están archivados/validados; periféricos presentes reportaron inválido/no disponible y la microSD no montó; SIM800L y clasificador acústico experimentales |

## Garantías y problemas conocidos

- Solo telemetría v3 usa spool durable en microSD y replay tras reconexión. Acústica
  y eventos se publican en vivo y se archivan en JSONL, pero no tienen replay durable.
- `/api/telemetry/basic` del ESP32 incluye GPS textual; `/api/status` omite GPS.
- WebSocket entrega cambios en vivo, no historial; al reconectar se debe refrescar REST.
- `DevicesPage` y `VehicleDetailPage` muestran “Producción” cuando
  `simulated=false`; ese flag solo significa no simulado. La etiqueta correcta debe
  ser “Real/no simulado” hasta disponer de procedencia productiva verificable.
- Los viajes se infieren por GPS. No existen ignición, OBD, combustible ni diagnóstico
  certificado.
- dBFS es relativo y el clasificador `heuristic-4` no tiene precisión validada.
- SIM800L depende de 2G, alimentación y TLS del entorno; no es ruta aceptada de
  producción.
- No hay política automatizada de retención ni prueba de restore/operación real.

## Próxima tarea recomendada

Completar la línea base física con microSD y sensores conectados: inducir corte y
restauración WiFi, reinicio con cola pendiente, replay, deduplicación y LWT
`offline → online`. Después validar comandos/acuses. El transporte básico
**ESP32 → NTP → HiveMQ → backend/PostgreSQL** ya tiene evidencia productiva.
