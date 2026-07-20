# VehicleSense simulator

Estado: contrato definido; implementación prevista junto con el backend.

El simulador permitirá desarrollar sin ESP32 físico y probar múltiples
vehículos. Publicará los mismos contratos MQTT que el firmware con
`simulated=true` y credenciales/tópicos autorizados para identidades de demo.

Escenarios mínimos:

- recorrido GPS y pérdida de fix;
- vehículo en movimiento, detenido, stale y offline;
- variaciones ambientales e IMU;
- categorías acústicas, clipping y micrófono inválido;
- alertas y viajes controlados;
- desconexión, cola, replay y mensajes duplicados;
- schema inválido para pruebas negativas opt-in.

El simulador no reemplazará silenciosamente datos de producción. La UI y la
base de datos conservarán la marca de simulación.
