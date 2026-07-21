# Simulador VehicleSense

Genera vehículos reproducibles sin un ESP32 físico y publica exactamente los
contratos JSON Schema de producción. Todos los mensajes incluyen
`simulated=true`, y las identidades usan los prefijos `sim-vehicle-` y
`sim-device-`. El backend, la base y la interfaz conservan esa marca: una
muestra simulada nunca sustituye silenciosamente una medición real.

## Capacidades

- Uno a 100 dispositivos MQTT independientes, cada uno con client ID, sesión y
  Last Will propios.
- Movimiento GPS reproducible, paradas, pérdida de fix y una desconexión
  controlada en el escenario `mixed`.
- DHT11, BMP180, BH1750, IMU, microSD, conectividad y acústica sintéticos.
- Categorías acústicas, micrófono inválido y eventos periódicos.
- Cola en memoria acotada, descarte del dato más antiguo y replay marcado.
- QoS 1, TLS con verificación de certificado y suscripción a comandos.
- Duplicados y mensajes inválidos únicamente mediante opciones explícitas para
  probar deduplicación y cuarentena del backend.

No simula la precisión física de los sensores ni demuestra que el clasificador
acústico funcione en un vehículo real.

## Instalación

Requiere Python 3.12 o 3.13 y [uv](https://docs.astral.sh/uv/):

```bash
cd simulator
uv sync --extra dev
cp .env.example .env
```

La corrida seca no necesita credenciales ni conexión:

```bash
uv run vehiclesense-simulator \
  --dry-run --cycles 5 --vehicles 3 --interval 0.1 --scenario mixed
```

Para inspeccionar los JSON completos use `--print-payloads`.

## HiveMQ Cloud

Complete `.env` localmente. Nunca confirme ese archivo en Git. El usuario MQTT
del simulador debe tener solamente estos permisos:

```text
PUBLISH   vehiclesense/v1/vehicles/sim-vehicle-+/devices/sim-device-+/telemetry
PUBLISH   vehiclesense/v1/vehicles/sim-vehicle-+/devices/sim-device-+/status
PUBLISH   vehiclesense/v1/vehicles/sim-vehicle-+/devices/sim-device-+/events
PUBLISH   vehiclesense/v1/vehicles/sim-vehicle-+/devices/sim-device-+/acoustic
PUBLISH   vehiclesense/v1/vehicles/sim-vehicle-+/devices/sim-device-+/command-acks
SUBSCRIBE vehiclesense/v1/vehicles/sim-vehicle-+/devices/sim-device-+/commands
```

El broker debe usar TLS en el puerto 8883. Si no se indica un CA local, Python
usa el almacén de certificados del sistema. Ejecución real:

```bash
uv run vehiclesense-simulator --vehicles 3 --scenario mixed
```

Cada comando recibido se valida contra `command-v1.schema.json`, se comprueba
contra la identidad del dispositivo y se responde en `command-acks`. Un comando
vencido obtiene estado `expired`.

## Escenarios y pruebas negativas

- `normal`: datos válidos continuos.
- `mixed`: paradas, GPS o micrófono inválido y pérdida temporal de un dispositivo.
- `stress`: temperatura alta controlada para ejercitar alertas.

Opciones útiles:

```bash
# Repetir una telemetría cada 10 ciclos con el mismo sample_id
uv run vehiclesense-simulator --duplicate-every 10

# Enviar deliberadamente un JSON fuera de contrato cada 20 ciclos
uv run vehiclesense-simulator --inject-invalid-every 20
```

La inyección inválida es una prueba negativa visible en consola; nunca está
activa por defecto.

## Validación

```bash
uv run ruff check .
uv run ruff format --check .
uv run pytest -q
```

Las pruebas cubren contratos, omisión de campos dependientes cuando GPS o
micrófono son inválidos, estados offline/online, comandos, ejecución CLI y el
límite/replay de la cola. La conexión real a HiveMQ requiere credenciales y se
valida como prueba manual separada.
