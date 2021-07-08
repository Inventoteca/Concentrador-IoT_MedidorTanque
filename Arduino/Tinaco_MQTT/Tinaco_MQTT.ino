/* 
 *Código para lectura de un tanque de agua y envío de 
 *la información por MQTT a un servidor preseleccionado
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Ajustar de acuerdo a tu red WiFi
//Modo STA
const char* ssidSTA1 = "Inventoteca_2G";
const char* passwordSTA1 = "science_7425";
const char* ssidSTA2     = "Inventoteca_2G"; //Por default, todos los dispositivos siempre podrán conectarse a Invento
const char* passwordSTA2 = "science_7425";
//Modo AP
const char *ssid = "SensorTanque_01";  // Generará esta red como Access Point
const char *pw = "12345678"; // con esta contraseña
IPAddress ip(192, 168, 0, 1); // Se asignará esta IP desde el Access Point
IPAddress netmask(255, 255, 255, 0);
const int port = 1881; // y este puerto

//Configuración MQTT
const char* mqtt_server = "broker.mqtt-dashboard.com";
//const char*  topicString = "Inventoteca/IoT/SensorAgua";
const char*  topicString = "neoNumber";

//Configuración Ultrasonico
const int pinecho = 4;
const int pintrigger = 5;

//CALIBRACIONES
//Se utilizan para la funcion PorcentajeDeposito()
int distMin=17; //Calibrar esto de acuerdo al tamaño del tanque de agua con medidas en cm,
int distMax=120; //se puede obtener de la lectura del sensor en puerto serial


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

int WifiTimeout = 15000; //Tiempo dedicado a conectarte a las redes
int distancia;
//----------------------------------------------SETUP---------------------------------------------------------//

void setup() {
  Serial.begin(115200);
  
  pinMode(pinecho, INPUT); //US
  pinMode(pintrigger, OUTPUT); //US}
  pinMode(LED_BUILTIN, OUTPUT); 
  
  setupWifi();
  
  delay(500);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

//----------------------------------------------LOOP---------------------------------------------------------//

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  int porcent = PorcentajeTanque();
  //PrintMediciones(x);
  
  unsigned long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    snprintf (msg, MSG_BUFFER_SIZE, "%ld%%", porcent);
    Serial.print("Publish message: ");
    Serial.print(msg);
    Serial.print(" to topic: ");
    Serial.println(topicString);
    
    client.publish(topicString, msg);
  }         
}

//----------------------------------------OTRAS FUNCIONES-------------------------------------------------------//

void setupWifi() {

  //Modo AP
  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP 
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP
  //server.begin(); // start TCP server
  Serial.println((String)"SSID: " + ssid + "  PASS: " + pw);
  Serial.println((String)"Connect to " + ip.toString() + ":" + port);
  Serial.println("");

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssidSTA1);
  WiFi.begin(ssidSTA1, passwordSTA1);
  while (WiFi.status() != WL_CONNECTED) { //Minibucle para revisar conexiones WiFi
    delay(500);
    Serial.print(".");
    if(millis()>=WifiTimeout) {
      Serial.println("Failed to connect to STA1");
      Serial.print("Connecting to ");
      Serial.println(ssidSTA2);
      WiFi.begin(ssidSTA2, passwordSTA2);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(millis()>=WifiTimeout*2) {
          Serial.println("Failed to connect to STA2");
          Serial.println("Vamonos al void loop().");
          return;
        }
      }
    }
  }
  
  // Print local IP address
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
  delay(500);
}

String getValue(String data, char separator, int index) //Para MQTT
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//Para MQTT
void callback(char* topic, byte* payload, unsigned int length) { //Recibe los mensajes un caracter a la vez, los une
  String message = "";

  Serial.print("Message arrived ["); Serial.print(topic); Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  String thistopic = String(topic);


  if (thistopic == topicString) {

    //r = getValue(message, ',', 0); //Separa lo primero hasta la primer coma
    //targetBlue = b.toInt();

  }
}

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
//      // Once connected, publish an announcement...
//      client.publish("outTopic", "hello world");
//      // ... and resubscribe
//      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int PorcentajeTanque(void){
  unsigned int tiempo;
  
  //Medición ultrasónico
  digitalWrite(pintrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pintrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pintrigger, LOW);
  tiempo = pulseIn(pinecho, HIGH);
  distancia = tiempo*0.034/2; //Constante de la velocidad del sonido

  float rango = distMax - distMin;
  int porcentaje = 100 - (distancia - distMin)*(100/rango);
  return porcentaje;
  
}

void PrintMediciones(int x){
  Serial.print("Medicion: ");
  Serial.print(distancia); //Imprime la medición actual
  Serial.println(" cm");
  Serial.print("Porcentaje actual del tanque: ");
  Serial.print(x);
  Serial.println("%");
  Serial.println("");
}
