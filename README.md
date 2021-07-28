# Concentrador-IoT_MedidorTanque
Repo para almacenar avances de programación para el medidor IoT para tanques de agua.

Se están construyendo 3 disppositivos:  
El primero es un medidor de nivel de agua para un contenedor. Usa un sensor ultrasónico a prueba de agua.  
El segundo es un medidor de temperatura. Usa un termopar con módulo MAX6675.  
El tercero es el gateway

Componentes de los dispositivos  
+ ESP8266 (módulo ESP-07)
+ RFM95W (LoRa)
+ Sensor ultrasónico [modelo]
+ MAX6675 y termopar

Se reutilizaron placas de Serverus. Se removieron los componentes inecesarios (regulador de voltaje, relevador).  
Para ahorrar un poco de energía se removió el LED indicador de encendido en los ESP-07 (LED rojo).  
Tenemos 2 tipos de módulos LoRa. Uno es el RFM95W de HopeRF y el otro es la versión modificada de Adafruit.
El módulo de Adafruit tiene un pin para deshabilitar (corta alimentación del radio),
regulador de voltaje y levelshifter. ¿Cuál de los 2 consume menos energía?  
https://www.hoperf.com/modules/lora/RFM95.html  
https://www.adafruit.com/product/3072  

Consumo de energía de RFM95W en tabla 51, página 13/121  
https://cdn.sparkfun.com/assets/learn_tutorials/8/0/4/RFM95_96_97_98W.pdf  

## Librerías
Para la comunicación LoRa se ha utilizado LoRaNow https://github.com/ricaun/LoRaNow  

Se usa deepSleep en el ESP8266. Para que el contador de mensajes 
de la librería no se reinicie se guarda el valor en la RTC memory.  
Modificación en la línea 500 del archivo `LoRaNow.cpp`  
```cpp
uint32_t _x; //variable temporal
ESP.rtcUserMemoryRead(0, &_x, sizeof(_x)); //leer valor
_count = ++_x; //incrementar
ESP.rtcUserMemoryWrite(0, &_x, sizeof(_x)); //guardar valor
```
Se puede modificar el tipo de dato de la variable `_count`.
Se encuentra definida en la línea 128 del archivo `LoRaNow.h`.  

Otras opciones son  
AirSpayce - Radiohead http://www.airspayce.com/mikem/arduino/RadioHead/  
LowPowerLab - RFM69 https://github.com/LowPowerLab/RFM69  

Para el módulo MAX6675 se ha utilizado la librería de Adafruit https://github.com/adafruit/MAX6675-library  

## Mongoose
La carpeta `Mongoose` contiene pruebas realizadas con el RTOS Mongoose.  

## Arduino
La carpeta `Arduino` contiene código usado en los prototipos y algunas pruebas.  
+ Código del tinaco solo con comuicación MQTT, después usará LoRa  
+ Ejemplos ejemplos de LoRaNow probados en ESP8266 (Gateway y Node)
+ El programa Node modificado para enviar lecturas de termocople con MAX6675.

Código extra  
+ Programa para Serverus instalado en la puerta de sala 1. Usa MQTT.
+ Programa para Serverus que se instalará en granja de impresoras.
  Usa 2 termocoples y comuicación MQTT.

## Notas
Los mensajes enviados por LoRa llevan 3 componentes:  
+ Payload
+ Id del nodo
+ Contador

El payload reporta varias lecturas. Se propone este formato  
`magnitud1=valor1,magnitud2=valor2`  
Por ejemplo, el nodo que mide temperatura envía un payload de esta forma  
`vcc=3.21,temperatura=28`  

El gateway recibe el mensaje por LoRa y lo reenvía al servidor de Inventoteca por MQTT.  
El mensaje LoRa se separa con la función strtok  
https://www.cplusplus.com/reference/cstring/strtok/  

Otras funciones útiles  
https://www.cplusplus.com/reference/cstdio/sprintf/  
https://www.cplusplus.com/reference/cstdio/printf/  

Una mejor forma de enviar los mensajes podría ser con formato JSON RPC  
https://www.jsonrpc.org/specification  
