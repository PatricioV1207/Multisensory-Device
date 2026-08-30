# Estado actual de VehicleSense

Fecha de corte: **2026-08-30**. Este estado describe lo observable en el repositorio;
no acredita hardware ni infraestructura externa.

## Línea base y evidencia

- Base auditada: rama `main`, commit `d5d2509` más cambios locales preservados de la
  migración documental AWS y la especificación PCB.
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
- La primera invocación global `python3 contracts/validate_fixtures.py` falló porque
  ese intérprete no tenía `jsonschema`. La ejecución
  `backend/.venv/bin/python contracts/validate_fixtures.py` pasó; fue un requisito
  de entorno, no un fallo de los contratos.
- El repositorio no contiene evidencia fechada de placa física, HiveMQ real,
  broker real, PostgreSQL real, Docker runtime/E2E, EC2 aprovisionada ni operación
  productiva.

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
| Despliegue | Parcial/documentado | Compose, Nginx, migraciones de arranque y scripts canónicos `backup_postgres.sh`/`restore_postgres.sh` validables estáticamente | AWS EC2 es una guía de referencia, no una producción aprovisionada; sin build/run E2E, restore real, monitorización ni HA; OCI es alternativa histórica |
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

Abrir un chat nuevo para **preparar y ejecutar una línea base E2E reproducible con
simulador, HiveMQ Cloud y PostgreSQL real**, incluyendo migraciones, deduplicación,
cuarentena, REST/WS, roles y backup/restore. Registrar prerequisitos y resultados sin
mezclarla todavía con la validación física del ESP32.
