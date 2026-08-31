# Reglas permanentes para agentes

Estas reglas aplican a cualquier chat que modifique este proyecto. El repositorio
debe poder trabajarse mediante tareas pequeñas e independientes.

## Contexto mínimo antes de actuar

1. Lea [`STATUS.md`](STATUS.md) y [`ROADMAP.md`](ROADMAP.md).
2. Lea [`docs/architecture.md`](docs/architecture.md), el documento especializado
   del área y el código/configuración que va a tocar.
3. Revise `git status --short` y el diff del archivo. El worktree puede contener
   trabajo del usuario o de otros agentes: consérvelo y no revierta cambios ajenos.
4. Verifique los nombres vigentes en código antes de citar perfiles, tópicos,
   schemas, rutas REST/WebSocket, migraciones o archivos de despliegue.

La prioridad de fuentes es: **código ejecutable, configuración, migraciones y
contratos; luego pruebas; luego documentación vigente**. Un Markdown nunca
sobrescribe al código.

## Alcance y verificación

- Mantenga cada chat en una sola tarea acotada. Si el cambio cruza subsistemas,
  explique el límite y divídalo en planes independientes.
- Ejecute las pruebas mínimas del área modificada y registre el comando y resultado
  reales. No atribuya una prueba previa al cambio actual.
- No borre caches, entornos, `node_modules`, `.pio`, informes locales ni artefactos
  ignorados salvo que el usuario lo pida explícitamente.
- Actualice `STATUS.md`, `ROADMAP.md` o la documentación especializada cuando el
  cambio altere un hecho persistente, una interfaz, un riesgo o el próximo paso.
- No agregue ADR ni otro documento si la decisión cabe en arquitectura, estado o
  el documento especializado existente.

## Cómo describir evidencia

Distinga siempre:

- **implementado:** existe código/configuración;
- **validado automáticamente:** una prueba ejecutada cubre el comportamiento;
- **validado físicamente:** existe evidencia fechada de hardware real;
- **validado en producción:** existe evidencia fechada en la infraestructura real.

No use “completo”, “seguro”, “probado” o “producción” sin precisar el alcance y la
evidencia. Simulación, compilación y validación estática no demuestran sensores,
broker, PostgreSQL ni despliegue reales.

## Seguridad y hardware

- Nunca publique `include/secrets.h`, `.env`, llaves, tokens, certificados o dumps.
- Mantenga credenciales separadas y ACL MQTT de mínimo privilegio por identidad.
- No debilite TLS ni active MQTT celular inseguro como fallback implícito.
- Antes de cablear, desconecte alimentación y siga `docs/wiring.md`; SIM800L usa
  fuente externa capaz de soportar sus picos y GND común.
- OTA local no está firmada, no usa HTTPS y no tiene rollback automático. Conserve
  USB como recuperación y no exponga el AP a Internet.
- Este proyecto es un prototipo académico: no lo presente como sistema de seguridad
  vehicular, medición acústica certificada ni diagnóstico automotriz.

## Cierre de una tarea

Informe archivos cambiados, pruebas ejecutadas y brechas restantes. Si aparece un
hecho que contradice estos documentos, corrija primero la afirmación persistente o
déjela marcada explícitamente como pendiente de verificación.
