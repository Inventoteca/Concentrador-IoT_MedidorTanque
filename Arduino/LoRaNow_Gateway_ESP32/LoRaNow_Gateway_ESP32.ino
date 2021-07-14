/*
  LoRaNow Gateway con ESP32

  Probado en Heltec WiFi LoRa 32 V2
  https://heltec.org/project/wifi-lora-32/
  
  Recibe mensajes de nodos LoRa y los publica por MQTT

  Se conecta automáticamente a la red de Inventoteca,
  pero en el futuro deberá generar una red WiFi para conectar
  otro dispositivo y hacer la configuración inicial.

  Basado en estos ejemplos:
    LoRaNow_Gateway_ESP32 (LoraNow) [by Luiz H. Cassettari]
    mqtt_esp8266 (PubSubClient)
*/

#include <WiFi.h>
#include <LoRaNow.h>
#include <PubSubClient.h>

// Por ahora no se necesita un server
//#include <WebServer.h>
//#include <StreamString.h>
//WebServer server(80);
//static StreamString string;
//const char *script = "<script>function loop() {var resp = GET_NOW('loranow'); var area = document.getElementById('area').value; document.getElementById('area').value = area + resp; setTimeout('loop()', 1000);} function GET_NOW(get) { var xmlhttp; if (window.XMLHttpRequest) xmlhttp = new XMLHttpRequest(); else xmlhttp = new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET', get, false); xmlhttp.send(); return xmlhttp.responseText; }</script>";

// La red de Inventoteca (esto debe ser configurable desde una app)
const char *ssid = "Inventoteca_2G";
const char *password = "science_7425";
const char* mqtt_server = "broker.mqtt-dashboard.com";

// Variables para MQTT
WiFiClient espClient;
PubSubClient client(espClient);
//unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
//int value = 0;

void setup(void)
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); //modo cliente
  //if (ssid != "") //ya sabemos que no está vacía
  WiFi.begin(ssid, password); //conectar a la red
  WiFi.begin();
  Serial.println();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  //randomSeed(micros()); //opcional

  // Imprimir info de conexión
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Iniciar MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //server.on("/", handleRoot);
  //server.on("/loranow", handleLoRaNow);
  //server.begin();
  //Serial.println("HTTP server started");

  // LoRaNow.setFrequencyCN(); // Select the frequency 486.5 MHz - Used in China
  // LoRaNow.setFrequencyEU(); // Select the frequency 868.3 MHz - Used in Europe
  LoRaNow.setFrequencyUS(); // Select the frequency 904.1 MHz - Used in USA, Canada and South America
  // LoRaNow.setFrequencyAU(); // Select the frequency 917.0 MHz - Used in Australia, Brazil and Chile

  // LoRaNow.setFrequency(frequency);
  // LoRaNow.setSpreadingFactor(sf);
  // LoRaNow.setPins(ss, dio0);

  // LoRaNow.setPinsSPI(sck, miso, mosi, ss, dio0); // Only works with ESP32

  // Iniciar LoRa
  if (!LoRaNow.begin())
  {
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ;
  }

  LoRaNow.onMessage(onMessage);
  LoRaNow.gateway();
}

void loop(void)
{
  LoRaNow.loop();
  //server.handleClient();
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

// Función que se ejecuta cuando llega un mensaje por MQTT
// (tal vez no se utiliza)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

// Función que se ejecuta cuando llega un mensaje por LoRa
// El mensaje que llega se retransmite por MQTT
void onMessage(uint8_t *buffer, size_t size)
{
  unsigned long id = LoRaNow.id();
  byte count = LoRaNow.count();

  Serial.print("Node Id: ");
  Serial.println(id, HEX);
  Serial.print("Count: ");
  Serial.println(count);
  Serial.print("Message: ");
  Serial.write(buffer, size);
  Serial.println();
  Serial.println();

  /*
    if (string.available() > 512)
    {
    while (string.available())
    {
      string.read();
    }
    }
  */
  /*
    string.print("Node Id: ");
    string.println(id, HEX);
    string.print("Count: ");
    string.println(count);
    string.print("Message: ");
    string.write(buffer, size);
    string.println();
    string.println();
  */

  // Send data to the node
  LoRaNow.clear();
  LoRaNow.print("LoRaNow Gateway Message ");
  LoRaNow.print(millis());
  LoRaNow.send();
}

// Función para reconectarse a WiFi
// Se podría omitir o usar una rutina diferente
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/////////////////////////////
//// CÓDIGO NO UTILIZADO ////
/////////////////////////////

/*
  void handleRoot()
  {
  String str = "";
  str += "<html>";
  str += "<head>";
  str += "<title>ESP32 - LoRaNow</title>";
  str += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  str += script;
  str += "</head>";
  str += "<body onload='loop()'>";
  str += "<center>";
  str += "<textarea id='area' style='width:800px; height:400px;'></textarea>";
  str += "</center>";
  str += "</body>";
  str += "</html>";
  server.send(200, "text/html", str);
  }
*/

/*
  void handleLoRaNow()
  {
  server.send(200, "text/plain", string);
  while (string.available()) // clear
  {
    string.read();
  }
  }
*/
