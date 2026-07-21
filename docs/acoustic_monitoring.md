# Monitoreo acústico con INMP441

## Alcance

VehicleSense captura el INMP441 por I2S a 16 kHz y procesa la señal en el
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

`heuristic-1` produce `traffic`, `music`, `speech`, `engine`, `horn`, `siren`,
`wind`, `quiet` o `unknown`. Las reglas priorizan `unknown` cuando una sola
ventana no aporta evidencia suficiente. En particular, una ventana espectral
no demuestra por sí sola la modulación temporal de una sirena.

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

- Validación física del pinout y del canal izquierdo.
- Medición de precisión con un dataset balanceado.
- Características temporales específicas de sirena.
- Calibración dB SPL, si el caso de uso llega a exigirla.
- Replay topic-aware de agregados y eventos acústicos.
