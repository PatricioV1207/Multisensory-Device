# Informes VehicleSense

Este directorio contiene tres informes independientes, editables en LaTeX:

1. `Informe_1_Investigacion_Propuesta`: investigación inicial y propuesta técnica.
2. `Informe_2_Calibracion_Pruebas`: calibración, pruebas reales y plan verificable para ensayos pendientes.
3. `Informe_3_Comunicacion_Plataforma`: MQTT, almacenamiento, web local, OTA y plataforma cloud.

## Edición rápida

- Los datos de portada están centralizados en `common/datos.tex`.
- El estilo compartido está en `common/preamble.tex`.
- Cada informe tiene un `main.tex` autónomo y su PDF compilado.
- Las imágenes están en `recursos/` y se distinguen por origen.

## Integridad de la evidencia

- `recursos/evidencia_real/`: imágenes extraídas del documento entregado por el autor.
- `recursos/terminales/`: ejecuciones funcionales **simuladas**, generadas a partir del formato del firmware; muestran resultados PASS/PARCIAL, pero no deben presentarse como mediciones físicas.
- `recursos/web_cloud/`: capturas del frontend en modo demostración.
- `recursos/web_local/`: vistas documentales del HTML embebido con datos representativos.

El informe 2 usa rótulos visibles para separar evidencia real y validación simulada. GPS, microSD, WiFi y MQTT figuran como PASS SIMULADO; INMP441 y SIM800L como PARCIAL SIMULADO. Antes de una entrega académica se deben completar los campos entre corchetes y conservar la palabra ``simulado'' mientras no existan capturas físicas.

## Compilación

Desde la carpeta de cada informe:

```bash
pdflatex -interaction=nonstopmode main.tex
pdflatex -interaction=nonstopmode main.tex
```

También puede utilizarse Tectonic:

```bash
tectonic main.tex
```

No se modificó ningún archivo existente del firmware o de la plataforma web.
