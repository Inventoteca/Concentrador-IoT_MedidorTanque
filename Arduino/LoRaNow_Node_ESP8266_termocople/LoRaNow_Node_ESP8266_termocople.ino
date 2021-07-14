/*
  LoRaNow Simple Node

  This code sends message and listen expecting some valid message from the gateway

  created 01 04 2019
  by Luiz H. Cassettari
*/

#include <LoRaNow.h>
#define LORA_CS 2 //chip select del módulo LoRa (RFM95W)
#define LORA_G0 15 //DIO 0
#define LORA_EN 4 //enable
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

// Todo ocurre dentro de la función setup
// No se necesitan otros eventos porque
void setup() {
  // Iniciar con módulo LoRa desactivado
  pinMode(LORA_EN, OUTPUT);
  digitalWrite(LORA_EN, 0);

  //Serial.begin(115200);
  Serial.begin(74880);
  Serial.println("LoRaNow Nodo con termocople");

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
  LoRaNow.print(tem);
  LoRaNow.send();

  // Entra en modo sueño profundo
  ESP.deepSleep(10e6);
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
