# SIM800L, GPRS y MQTT

SIM800L usa UART1: TX del módem hacia GPIO16 y RX del módem desde GPIO17. Debe
tener fuente independiente capaz de entregar picos de al menos 2 A, condensador
de 470–1000 µF, antena y tierra común con ESP32.

No se controlan PWRKEY, RESET ni DTR porque no se asignaron GPIO. El firmware
reintenta por AT, pero no puede reiniciar físicamente un módem bloqueado.

En `secrets.h` se definen APN, PIN SIM opcional y credenciales MQTT. La máquina
de estados es:

```text
AT_SYNC -> SIM_READY -> REGISTERING -> GPRS_CONNECTING -> ONLINE
                                                      -> BACKOFF
```

Orden de habilitación:

1. `test_sim800l_at`: comunicación, SIM y señal.
2. `test_sim800l_gprs`: cobertura 2G, operador, APN, IP y una conexión TCP al
   host/puerto de diagnóstico `MQTT_TEST_*`.
3. `test_sim800l_mqtt`: diagnóstico sintético por TCP.
4. `test_sim800l_mqtt_tls`: broker real por TLS/SNI.
5. `full_prototype_cellular`.

La integración real usa TLS por defecto. Si SSL del SIM800L falla, el firmware
mantiene microSD y no baja automáticamente a texto plano. El override
`ALLOW_INSECURE_CELLULAR_MQTT` es solo para un broker privado de laboratorio.
