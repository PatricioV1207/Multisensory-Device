# Monitoreo acústico con INMP441

## Alcance

El firmware captura el INMP441 por I2S a 16 kHz y procesa la señal en el
ESP32. Publica características y categorías aproximadas, pero no audio crudo.
El objetivo de esta fase es instrumentar y recolectar evidencia para mejorar
el modelo, no presentar el clasificador heurístico como detector certificado.

## Procesamiento

Cada frame de 1024 muestras pasa por eliminación de DC, filtro pasa-altos de
primer orden a 90 Hz, ventana Hann y FFT radix-2. Dieciséis frames forman una
ventana agregada de aproximadamente 1.024 s. Se calculan:

- RMS y pico en dBFS relativo;
- relación de clipping y crest factor;
- tasa de cruces por cero;
- centroide, flatness y rolloff espectral;
- proporción de energía en 80–250, 250–800, 800–2000, 2000–4000 y
  4000–8000 Hz.

Una señal sin variación o una lectura I2S detenida invalida `mic_valid`; no se
reporta como un ambiente silencioso válido. Los números no finitos o fuera del
contrato se omiten del JSON.

## Clasificación y alertas

`heuristic-4` produce `traffic`, `music`, `speech`, `engine`, `horn`, `siren`,
`wind`, `quiet`, `noise` o `unknown`. `unknown` queda reservado para entrada
inválida, sin respuesta, no finita o con clipping. Una señal válida que no
encaja en una firma específica se asigna a `speech` con confianza baja o a la
categoría neutral `noise`; ninguna de las dos puede crear una alerta acústica.
En particular, una ventana espectral no demuestra por sí sola la modulación
temporal de una sirena.

La primera calibración física dejó el umbral provisional de silencio en
-88 dBFS. Añadió dos firmas conservadoras de claxon observadas desde el interior:
una para un vehículo posterior, con energía dominante entre 2 y 4 kHz, y otra
para el claxon del propio vehículo, con energía dominante entre 250 y 800 Hz.
Ambas devuelven confianza menor que 0.68, de modo que sirven para recopilar y
revisar categorías pero todavía no activan alertas por sí solas.

Los candidatos de voz priorizan energía entre 250 y 2000 Hz, contenido de
presencia hasta 4 kHz y espectro moderadamente tonal. Los candidatos de tráfico
priorizan ruido más ancho, mayor cruce por cero o energía por encima de 2 kHz,
y exigen más de -45 dBFS. Ese límite conservador evita llamar tráfico al fondo
de un cuarto con una impresora 3D, observado entre aproximadamente -89.6 y
-65.8 dBFS. Esta separación no demuestra precisión frente a tráfico vehicular
real y debe revisarse con muestras etiquetadas dentro del automóvil.

Solo `traffic`, `horn` y `siren` pueden convertirse en eventos. De forma
predeterminada requieren nivel mínimo de -40 dBFS, confianza de 0.68,
persistencia de 8 s y ausencia de clipping. Existe un cooldown de 30 s; si la
condición continúa, puede emitirse un nuevo evento al terminar cada cooldown.

Estos umbrales son iniciales y no representan métricas de precisión. Antes de
usarlos para decisiones reales deben evaluarse con un dataset etiquetado del
interior y exterior de los vehículos objetivo, incluyendo falsos positivos y
negativos.

## Privacidad y unidades

`ACOUSTIC_RAW_AUDIO_ENABLED` permanece en cero. La microSD de producción
recibe agregados JSON y eventos; el environment de colección guarda filas de
características en `/acoustic/features.jsonl`. Ninguno guarda PCM, voz o clips.

dBFS expresa amplitud relativa al máximo del convertidor digital. No equivale
a dB SPL. Convertirlo a presión sonora requiere una fuente acústica calibrada,
geometría reproducible y una curva de sensibilidad/corrección del conjunto
micrófono, montaje y carcasa.

## Entrega

- La telemetría v3 incluye un resumen acústico reciente.
- El tópico `acoustic` recibe agregados `acoustic-v1` mediante QoS 1.
- El tópico `events` recibe alertas `event-v1` mediante QoS 1.
- Ambos se archivan en JSONL si la microSD está disponible.
- El spool durable actual reenvía telemetría v3, no los tópicos acústicos
  independientes. El backend debe tolerar duplicados QoS 1 por identificador.

## Limitaciones pendientes

- Repetir la validación del selector I2S en otras unidades ESP32/INMP441.
- Medición de precisión con un dataset balanceado.
- Características temporales específicas de sirena.
- Calibración dB SPL, si el caso de uso llega a exigirla.
- Replay topic-aware de agregados y eventos acústicos.
