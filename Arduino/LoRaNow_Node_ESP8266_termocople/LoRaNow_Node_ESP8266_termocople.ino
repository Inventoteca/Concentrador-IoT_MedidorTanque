/*
  LoRaNow Node con termocople

  Este código envía lecturas de temperatura.
  Hardware: ESP8266, RFM95W, Termocople con MAX6675

  El nodo debe tener un id único. Para generarlo tenemos estas opciones:
  Usar la dirección MAC
  Usar el id generado por la biblioteca LoRaNow.
    Este id se genera con la func. makeId(). Es un entero de 32 bits
    En el ESP8266 se usa el chip id, obtenido con la func. ESP.getChipId()
    Podría no ser único, pero sirve por ahora
    https://github.com/esp8266/Arduino/issues/921
    https://bbs.espressif.com/viewtopic.php?t=1303

    https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#esp-specific-apis

    ESP.getEfuseMac() solo funciona en ESP32
    WiFi.macAddress()

  La librería LoRaNow envía 3 componentes en cada mensaje:
  1. Payload
  2. ID
  3. Count
  La variable count incrementa cuando se envía un mensaje
  (línea 500 en LoRaNow.cpp). Se debe modificar la librería para enviar
  un valor almacenado en la RTC memory.
  
  El gateway reenvía los mensajes recibidos por MQTT. Usa tópicos con esta forma
  "concentrador/ESPXXXXXX/magnitud"
  La palabra concentrador no cambia
  El id comienza con ESP y XXXXXX son los 3 bytes finales de la MAC en formato hexadecimal
  Por último va el nombre de la magnitud que se está leyendo. Ej.: temperatura, humedad, distancia, etc.

  Es posible recibir mensajes del gateway, pero no está implementado.

  Basado en ejemplo LoRaNow_Node (LoRaNow) [by Luiz H. Cassettari]
*/

//#include <ESP8266WiFi.h> //para obtener la dirección MAC
ADC_MODE(ADC_VCC); //esto es para leer el voltaje de entrada con el ADC
#define T_SLEEP 5*60e6 //tiempo que permanece en Deep Sleep (microsegundos) [5 minutos]

#include <LoRaNow.h>
#define LORA_CS 2 //chip select del módulo LoRa (RFM95W)
#define LORA_G0 15 //DIO 0
#define LORA_EN 4 //enable (este pin solo está disponible en el módulo de Adafruit)
// Otros pines se comparten con el módulo MAX6675 (puerto SPI)
// SCK 14
// MISO 12
// MOSI 13

#include <max6675.h>
#define DO 12 //MISO (salida de datos)
#define SCK 14 //reloj
#define CS 5 //chip select
MAX6675 termo(SCK, CS, DO);
// En una versión anterior de la librería se modificó el archivo max6675.cpp para funcionar con ESP8266
// https://github.com/adafruit/MAX6675-library/issues/9
// línea 8 agregada #define _delay_ms(ms) delayMicroseconds((ms) * 1000)
// línea 10 comentada //#include <util/delay.h>
// En la versión más reciente (julio 2021) ya no es necesario

// Todo ocurre dentro de la función setup()
// No se necesitan otros eventos porque el chip 
// entra en modo Deep Sleep y luego se reinicia.
void setup() {
  // Iniciar con módulo LoRa desactivado
  pinMode(LORA_EN, OUTPUT);
  digitalWrite(LORA_EN, 0);

  //Serial.begin(115200);
  Serial.begin(74880);
  Serial.println("LoRaNow Nodo con termocople");
  
  //Serial.println(RF_DISABLED);
  //Serial.println(WAKE_RF_DISABLED);

  // Comparar chipId y MAC
  // La dirección MAC usa 6 bytes
  // El chipId está formado por los 3 últimos bytes de la MAC
  /*
  uint32_t chipId = ESP.getChipId();
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.println(chipId, HEX);
  Serial.print(mac[0], HEX);
  Serial.print(mac[1], HEX);
  Serial.print(mac[2], HEX);
  Serial.print(mac[3], HEX);
  Serial.print(mac[4], HEX);
  Serial.println(mac[5], HEX);
  */

  // LoRaNow.setFrequencyCN(); // Select the frequency 486.5 MHz - Used in China
  // LoRaNow.setFrequencyEU(); // Select the frequency 868.3 MHz - Used in Europe
  LoRaNow.setFrequencyUS(); // Select the frequency 904.1 MHz - Used in USA, Canada and South America
  // LoRaNow.setFrequencyAU(); // Select the frequency 917.0 MHz - Used in Australia, Brazil and Chile

  // LoRaNow.setFrequency(frequency);
  // LoRaNow.setSpreadingFactor(sf);

  // LoRaNow.setPins(ss, dio0);
  // Pines por defecto en ESP8266: ss-GPIO16-D0, dio0-GPIO15-D8
  // Debemos dejar libre el GPIO16 para usar deepSleep
  // Quedan disponibles GPIO 2, 4 y 5
  //LoRaNow.setPins(D4, D8); //pines disponibles en módulo para pila 18650
  LoRaNow.setPins(LORA_CS, LORA_G0);

  // LoRaNow.setPinsSPI(sck, miso, mosi, ss, dio0); // Only works with ESP32

  // Leer temperatura (grados Celcius)
  float tem = termo.readCelsius();
  Serial.print("C = ");
  Serial.println(tem);

  // Leer voltaje de alimentación (VCC)
  float vcc = ESP.getVcc();
  vcc *= 3.283 / 3514.0; //Factor de conversión a volts

  // Activar módulo LoRa
  digitalWrite(LORA_EN, 1);
  if (!LoRaNow.begin()) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }

  // ¿Qué pasa si no asignamos las siguientes funciones?
  //LoRaNow.onMessage(onMessage);
  //LoRaNow.onSleep(onSleep);
  //LoRaNow.showStatus(Serial);

  // Enviar lectura
  LoRaNow.print("temperatura=");
  LoRaNow.print(tem);
  LoRaNow.print(",vcc=");
  LoRaNow.print(vcc);
  LoRaNow.send();

  // Entra en modo sueño profundo y deshabilitar WiFi
  ESP.deepSleep(T_SLEEP, WAKE_RF_DISABLED);
  //Existe otra constante con el mismo valor
  //WAKE_RF_DISABLED = RF_DISABLED = 4
}

void loop() {
  LoRaNow.loop();
}

/*
  void onMessage(uint8_t *buffer, size_t size)
  {
  Serial.print("onMessage: ");
  Serial.write(buffer, size);
  Serial.println();
  Serial.println();
  }
*/

/*
  void onSleep()
  {
  Serial.println("onSleep");
  delay(5000); // "kind of a sleep"
  //Serial.println("Send Message");
  //LoRaNow.print("LoRaNow Node Message ");
  //LoRaNow.print(millis());
  //LoRaNow.send();
  }
*/
