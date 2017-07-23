#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

/*====================================================================
 *                         IO Config
 *==================================================================== 
 */
 #define lampPin D0
 #define irSenderPin D1
 #define irReceiverPin D2

 IRsend irsend(irSenderPin);
 
uint16_t powerOffAc[67] = {9050,4450, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650,1650, 650,550, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650};  // NEC 20DF10EF
uint16_t powerOnAc[67] = {9050,4450, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650,1650, 650,550, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650};  // NEC 20DF10EF


/*====================================================================
 *                         WiFi Configuration
 *==================================================================== 
 */
const char* ssid = "SSID";
const char* password = "PASSWD";

/*====================================================================
 *                         MQTT Configuration
 *==================================================================== 
 */
const char* broker = "m11.cloudmqtt.com";
const int port = 14581;
const char* id = "test";
const char* user = "zqakfvdw";
const char* pw = "mSXZM1Lvajuw";

const char* topic_sub = "steaph/alaurentino/status";
const char* topic_pub = "steaph/alaurentino/temp";

WiFiClient esp;
PubSubClient client(esp);

void setup() {
  Serial.begin(115200);
  delay(10);
  
  pinMode(lampPin, OUTPUT); 
  pinMode(irSenderPin, OUTPUT);
  pinMode(irReceiverPin, INPUT);

  delay(20);
  setStatus(false);

  connectWifi();
  configMqtt();
}

void loop() {
  connectBroker();
  publish();
  
  client.loop();
}

/*====================================================================
 *                         WiFi Configuration
 *==================================================================== 
 */

void connectWifi()
{
  if (WiFi.status() == WL_CONNECTED)
        return;
 
  delay(20);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/*====================================================================
 *                         MQTT Configuration
 *==================================================================== 
 */

void configMqtt()
{
  client.setServer(broker, port);
  client.setCallback(mqtt_callback);
}

void connectBroker()
{
  connectWifi();
  
  while (!client.connected()) 
  {
    Serial.print("> Trying to connect to Broker:");
    Serial.println(broker);
    if (client.connect(id, user, pw)) 
    {
        Serial.println(">Succefuly connected to the Broker!");
        subscribe();
    } 
    else
    {
      Serial.println("\n> Faleire connection to Broker!");
      Serial.println("> Trying reconnection again in 2s");
      delay(2000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
    for(int i = 0; i < length; i++)
       msg += (char)payload[i];
       
    if(String(topic_sub).equals(String(topic))){
      Serial.print("msg:  ");
      Serial.println(msg);
      setStatus(msg != "false");
    }
}

void publish()
{
  float t = temperature();
  Serial.println(String("> Publishing the temperature: ") + String(t) + "˚C");
  client.publish(topic_pub, String(t).c_str());
}

void subscribe()
{
  Serial.println("\n> Subscribing the status...\n");
  client.subscribe(topic_sub, 0);
}

/*====================================================================
 *                      Sensors Configuration
 *==================================================================== 
 */

float temperature()
{
  float temp = (analogRead(A0) * 330.0f) / 1023.0f;
  return temp;
}

/*====================================================================
 *                            Actuador
 *==================================================================== 
 */

 void setStatus(bool status)
 {
  if(status)
  {
    Serial.println("> Environment enable!");
    digitalWrite(lampPin, LOW); // Inverse
     irsend.sendRaw(powerOnAc, sizeof(powerOnAc), 38);
    return;
  }

  Serial.println("> Environment disable!");
  digitalWrite(lampPin, HIGH); // Inverse
  irsend.sendRaw(powerOffAc, sizeof(powerOffAc), 38);
 }
