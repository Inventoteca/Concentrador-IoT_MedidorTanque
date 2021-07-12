/*
  LoRaNow Simple Node

  This code sends message and listen expecting some valid message from the gateway

  created 01 04 2019
  by Luiz H. Cassettari
*/

#include <LoRaNow.h>

#include <max6675.h>
#define DO  12 //salida de datos del termocople
#define SCK 14 //reloj
#define CS 2 //chip selectdel termocople
MAX6675 termo(SCK, CS, DO);
// El inicio del archivo max6675.cpp se modificó para funcionar con ESP8266
// https://github.com/adafruit/MAX6675-library/issues/9
// línea 8 agregada #define _delay_ms(ms) delayMicroseconds((ms) * 1000)
// línea 10 comentada //#include <util/delay.h>

uint64_t t; //tiempo actual (millis)
uint64_t tLectura = 0; //momento de hacer lectura
unsigned int p = 3000; //periodo entre lecturas

void setup() {
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
  //LoRaNow.setPins(D4, D8); //pines disponibles en módulo para pila 18650

  // LoRaNow.setPinsSPI(sck, miso, mosi, ss, dio0); // Only works with ESP32

  if (!LoRaNow.begin()) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }

  LoRaNow.onMessage(onMessage);
  LoRaNow.onSleep(onSleep);
  LoRaNow.showStatus(Serial);

  tLectura = millis(); //inicializar
}

void loop() {
  LoRaNow.loop();

  t = millis(); //leer tiempo actual
  if (t >= tLectura) {
    Serial.print("C = ");
    Serial.println(termo.readCelsius());
    tLectura += p; //sumar periodo
  }
}

void onMessage(uint8_t *buffer, size_t size)
{
  Serial.print("onMessage: ");
  Serial.write(buffer, size);
  Serial.println();
  Serial.println();
}

void onSleep()
{
  Serial.println("onSleep");
  delay(5000); // "kind of a sleep"
  Serial.println("Send Message");
  LoRaNow.print("LoRaNow Node Message ");
  LoRaNow.print(millis());
  LoRaNow.send();
}
