# Roadmap del proyecto

Estado al 2026-08-31. Este archivo enumera solo etapas mayores; **cada bloque debe
recibir su propio plan en un chat nuevo**, después de releer `STATUS.md`, arquitectura
y código.

## 1. Establecer la línea base E2E y física real

Ejecutar y documentar ESP32 real → HiveMQ Cloud → backend con PostgreSQL → frontend,
incluyendo fallos de red, reinicio, deduplicación, replay de telemetría, roles,
backup/restore y una sesión física prolongada. Separar claramente resultados del
simulador y del hardware.

## 2. Validar hardware y PCB

La revisión física de cuatro capas y su caja cuentan con evidencia visual en el
informe final. Resta resolver los bloqueos de la especificación Rev A, archivar el
esquema/footprints/*stack-up*/Gerbers/BOM, revisar ERC/DRC y completar el *bring-up*
eléctrico, buses, sensores, alimentación y calibración con evidencia fechada antes
de declarar validada la integración o repetir una fabricación.

## 3. Completar comandos, autorización y operación

Implementar semántica segura e idempotente de comandos en firmware, sesiones refresh
revocables/logout, ACL por vehículo, historial de estado consultable, reglas de alerta
configurables y procedimientos operativos/retención acordes al riesgo.

## 4. Validar capacidades acústicas y celulares

Medir el clasificador con dataset real etiquetado y decidir si requiere calibración
dB SPL. Validar SIM800L solo como experimento o seleccionar un módem LTE con TLS y
cobertura mantenibles; ninguna ruta insegura debe convertirse en fallback.

## 5. Endurecer operación y observabilidad

Añadir límites internos de ingesta, métricas, logs y alertas operativas, limpieza de
retención, pruebas de restauración, rotación de secretos, firma/rollback OTA, escaneo
de dependencias e imágenes, corregir etiquetas UI que confunden “no simulado” con
“producción” y obtener evidencia de despliegue real. Evaluar alta disponibilidad solo
si el caso de uso la exige.
