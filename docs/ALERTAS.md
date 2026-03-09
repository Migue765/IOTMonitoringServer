# Cómo funcionan las alertas

## Flujo

1. **Servicio de control** (EC2 IoT Alert App, `manage.py start_control`):
   - Cada **5 minutos** ejecuta `analyze_data()`.
   - Consulta todos los datos de la **última hora** en la base de datos.
   - Para cada (estación, variable) calcula el **promedio** de esa hora.
   - Lee los límites de la variable: **min_value** y **max_value** (tabla `receiver_measurement`).
   - Si `promedio > max_value` **o** `promedio < min_value` → **envía alerta**.
   - La alerta se publica por MQTT al tópico: `país/estado/ciudad/usuario/in`.
   - Mensaje: `ALERT <variable> <min> <max>` (ej. `ALERT luminosidad 20 100`).

2. **Arduino** está suscrito a `.../in`. Cuando recibe un mensaje que contiene "ALERT", lo muestra en la pantalla durante unos segundos.

3. **Límites** de cada variable están en la base de datos (tabla `receiver_measurement`): campos `min_value` y `max_value`. Si no se han configurado, quedan en NULL y el control los trata como 0.

## Alertas por variable

- **Temperatura:** si quieres alerta cuando sube o baja de un rango, pon en la variable "temperatura" un Valor mínimo y un Valor máximo (ej. 18 y 30).
- **Humedad:** igual (ej. 30 y 80).
- **Luminosidad:** para "alerta si baja de 20 %", pon **Valor mínimo = 20** y **Valor máximo = 100** (o 99). Así, si el promedio de la última hora es **&lt; 20**, se envía la alerta.

No hace falta modificar código del control ni del Arduino para esto; solo configurar los límites de la variable en la web (o en la BD).
