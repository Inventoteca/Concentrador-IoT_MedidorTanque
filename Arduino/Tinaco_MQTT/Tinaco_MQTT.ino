/* 
 *Código para lectura de un tanque de agua y envío de 
 *la información por MQTT a un servidor preseleccionado 
 *usando DeepSleep para minimizar consumo
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>

// Ajustar de acuerdo a tu red WiFi
//Modo STA
const char* ssidSTA1 = "Inventoteca_2G";
const char* passwordSTA1 = "science_7425";
//Configuración MQTT
const char* mqtt_server = "iot.inventoteca.com";
const char* topicString = "ConcentradorIoT/Sensores/NivelAgua";

//Configuración Ultrasonico
const int pinecho = 4;
const int pintrigger = 5;

//CALIBRACIONES
//Se utilizan para la funcion PorcentajeDeposito()
int distMin=17; //Calibrar esto de acuerdo al tamaño del tanque de agua con medidas en cm,
int distMax=120; //se puede obtener de la lectura del sensor en puerto serial

unsigned long int tiempo_dormido = 55e6; //tiempo en microsegundos

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

//OTA
const char* host = "esp8266-webupdate";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


//RTC
#define RTCMEMORYSTART 65
#define MAXHOUR 10000// number of hours to deep sleep for
extern "C" {
#include "user_interface.h"
}
typedef struct {
  int count;
} rtcStore;
rtcStore rtcMem;

String MAC = WiFi.macAddress();
int distancia, loopCounter = 0;
ADC_MODE(ADC_VCC); //Para leer voltaje de la pila
//----------------------------------------------SETUP---------------------------------------------------------//

void setup() {
  Serial.begin(115200);
  pinMode(pinecho, INPUT); //US
  pinMode(pintrigger, OUTPUT); //US
  setupWifi();
  //MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //OTA
  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
 
}

//----------------------------------------------LOOP---------------------------------------------------------//

void loop() {      
  //MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //OTA
  //while(true){
  httpServer.handleClient(); 
  MDNS.update();    
  //}
 
  
  int porcent = PorcentajeTanque(); //La primer medición es BASURA
  porcent = PorcentajeTanque(); //Repetir medición
  
  if(porcent<0 || porcent>600) //Volver a hacer la medición si salió mal
  {
    delay(200);
    porcent = PorcentajeTanque(); 
    if(porcent<0 || porcent>600) //Volver a hacer la medición si salió mal
      {
        delay(200);
        porcent = PorcentajeTanque(); 
      }  
  }
  
  PrintMediciones(porcent);
  
  //Envío por MQTT
  snprintf (msg, MSG_BUFFER_SIZE, "%ld", porcent);
  Serial.print("Publish message: ");
  Serial.print(msg);
  Serial.print(" to topic: ");
  Serial.println(topicString);
  client.publish(topicString, msg);

  //Para saber el voltaje de la pila
  float volt = ESP.getVcc()/1000.0;
  Serial.print("Vcc read: ");
  Serial.print(volt);
  Serial.println("V");
  snprintf (msg, MSG_BUFFER_SIZE, "%.2f", volt);
  client.publish("ConcentradorIoT/Sensores/Volt", msg);

  //Para saber cuántos ciclos ha realizado
  int counter = readFromRTCMemory();
  snprintf (msg, MSG_BUFFER_SIZE, "%ld", counter);
  client.publish("ConcentradorIoT/Sensores/Contador", msg);

  //Para saber que está vivo y cuál es su MAC address
  int str_len = MAC.length() + 1;
  MAC.toCharArray(msg, str_len);
  client.publish("ConcentradorIoT/Alive", msg);
  
  loopCounter++; //Se usa un bucle porque necesitas mandar más de una vez los valores por MQTT para que sí lleguen, aparentemente

  if(loopCounter == 7){
    writeToRTCMemory();
    //Irse a dormir
    Serial.print("a dormir por ");
    Serial.print(tiempo_dormido/1000000);
    Serial.println("s");
    ESP.deepSleep(tiempo_dormido); //Bye bye  
  }

  delay(1000);
}

//----------------------------------------OTRAS FUNCIONES-------------------------------------------------------//

void setupWifi() {

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssidSTA1);
  WiFi.begin(ssidSTA1, passwordSTA1);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  
  // Print local IP address
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
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
////Para MQTT
void callback(char* topic, byte* payload, unsigned int length) { //Recibe los mensajes un caracter a la vez, los une
//  String message = "";
//
//  Serial.print("Message arrived ["); Serial.print(topic); Serial.print("] ");
//  for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
//    message += (char)payload[i];
//  }
//  Serial.println();
//
//  String thistopic = String(topic);
//
//
//  if (thistopic == topicString) {
//
//    //r = getValue(message, ',', 0); //Separa lo primero hasta la primer coma
//    //targetBlue = b.toInt();
//
//  }
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
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(3000); //!
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
//
//  Serial.print("time: ");
//  Serial.print(tiempo); //Imprime la medición actual
//  Serial.println(" s");
//  
//  Serial.print("Mediciooon: ");
//  Serial.print(distancia); //Imprime la medición actual
//  Serial.println(" cm");
  
  float rango = distMax - distMin;
  int porcentaje = 100 - (distancia - distMin)*(100/rango);
  return porcentaje;
  
}

void PrintMediciones(int x){

  Serial.print("Porcentaje actual del tanque: ");
  Serial.print(x);
  Serial.println("%");
  Serial.println("");
}

int readFromRTCMemory() {
  system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));

  Serial.print("count = ");
  Serial.println(rtcMem.count);
  yield();
  return rtcMem.count;
}

void writeToRTCMemory() {
  if (rtcMem.count <= MAXHOUR) {
    rtcMem.count++;
  } else {
    rtcMem.count = 0;
  }

  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, 4);
  yield();
}
