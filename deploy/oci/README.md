# Oracle Cloud Infrastructure: referencia histórica

VehicleSense adoptó AWS EC2 como proveedor productivo de referencia. Esta
carpeta conserva OCI como alternativa porque el despliegue Docker Compose es
portable entre hosts Ubuntu `arm64` y `amd64`.

Para usar OCI, aprovisione una VM Ubuntu 24.04, una IP pública estable y reglas
de entrada equivalentes a las de AWS:

- TCP 22 únicamente desde la IP de administración;
- TCP 80 y 443 desde Internet;
- ningún acceso público a 5432, 8000 u 8080;
- salida para DNS, HTTPS y MQTT/TLS TCP 8883.

Después use `deploy/scripts/bootstrap_ubuntu.sh` y las mismas instrucciones de
configuración, Certbot, backup y rollback descritas en la
[guía AWS](../aws/README.md), sustituyendo solamente el aprovisionamiento de la
instancia, la IP y el firewall del proveedor.

OCI no forma parte de la validación productiva actual. Antes de adoptarlo,
confirme disponibilidad, cuotas, costos y arquitectura de la instancia en la
documentación vigente de Oracle.
