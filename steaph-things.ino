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

WiFiClient esp;

void setup()
{
  Serial.begin(115200);
  delay(10);
  
  pinMode(lampPin, OUTPUT); 
  pinMode(irSenderPin, OUTPUT);
  pinMode(irReceiverPin, INPUT);

  delay(20);
  setStatus(false);

  connectWifi();
}

void loop()
{
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
