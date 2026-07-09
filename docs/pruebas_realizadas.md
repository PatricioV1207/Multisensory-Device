1. `full_prototype`: comprobar compilación base.
2. Serial Monitor: verificar banner sin reinicios.

3. `test_dht11`: lectura y desconexión. 
    - Pruebas de build, upload y monitor exitosas.
    - Lecturas estables pero sin comprobacion de valores correctos.

4. `test_gps`: caracteres NMEA y fix al aire libre.
5. `test_i2c_scanner`: anotar direcciones observadas.
    - Prueba existosa, se verifico las 4 direcciones del sensor gy801
    - [W][Wire.cpp:301] begin(): Bus already started in Master Mode, sale este error que se debe verificar mas adelante.


6. `test_adxl345`, `test_l3g4200d`, `test_hmc5883l`, `test_bmp180`.
    - Test del acelerometro adxl345: Se debe calibrar mejor los valores.


7. `test_gy801`: confirmar validez independiente.
    - Prueba existosa, se detectan los 4 sensores.


8. `test_wifi`: IP, RSSI y reconexión.
9. `test_mqtt_wifi`: observar el mensaje en el broker.
10. `test_payload_json` y después `full_prototype` durante al menos 30 minutos.
