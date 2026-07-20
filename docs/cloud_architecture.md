# Arquitectura cloud propuesta

La web cloud no forma parte del firmware de esta fase:

```text
ESP32 + SIM800L -> MQTT/TLS -> broker cloud
                                  |-> WSS read-only -> frontend Vercel
                                  `-> bridge MQTT -> Supabase -> historial
```

La web cloud mostrará GPS, mapa, ruta, sensores completos, gráficas e historial.
La vista local continuará sin GPS.

Para el prototipo se propone HiveMQ Cloud o EMQX Cloud como broker y Vercel
para el frontend. Ambos brokers cloud deben comprobarse primero con
`test_sim800l_mqtt_tls`: sus planes administrados usan TLS y EMQX Serverless
requiere además SNI. Los planes gratuitos y sus límites pueden cambiar, por lo
que deben verificarse antes del despliegue.

El navegador se conecta por WSS solamente para una vista en vivo y usa una
credencial exclusiva de lectura limitada por ACL. Nunca se expone la
credencial con permisos de publicación del bus ni una clave administrativa.

Para historial, un consumidor MQTT siempre activo valida el payload v2 y lo
escribe en Supabase; Vercel consulta esa base. El plan gratuito de Supabase
puede pausar proyectos inactivos. Vercel es adecuado para frontend y funciones
cortas, no para mantener un suscriptor MQTT permanente. Una instancia gratuita
de Render que se suspende tras inactividad HTTP tampoco es fiable como bridge
siempre activo.

AWS IoT Core queda como evolución profesional: exige TLS moderno y normalmente
certificados X.509 por dispositivo, lo que aumenta la complejidad y el riesgo
de compatibilidad con SIM800L. Si TLS/SNI no es estable, la recomendación es
migrar el transporte a SIM7600, no degradar la telemetría real a MQTT abierto.
