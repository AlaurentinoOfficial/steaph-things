#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ArduinoJson.h>

/*====================================================================
 *                         I/O Config
 *==================================================================== 
 */
#define lampPin D0
#define irSenderPin D1
#define irReceiverPin D2
#define pirPin D5
#define gasPin D7
#define tempPin D8

IRsend irsend(irSenderPin);
 
uint16_t powerOffAc[67] = {9050,4450, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650,1650, 650,550, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650};  // NEC 20DF10EF
uint16_t powerOnAc[67] = {9050,4450, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650,1650, 650,550, 650,550, 650,550, 650,1650, 650,550, 650,550, 650,550, 650,550, 650,1650, 650,1650, 650,1650, 650,550, 650,1650, 650,1650, 650,1650, 650,1650, 650};  // NEC 20DF10EF


/*====================================================================
                          WiFi Configuration
 *==================================================================== 
 */
const char* ssid = "ssid";
const char* password = "........";

/*====================================================================
 *                        MQTT Configuration
 *==================================================================== 
 */
char broker[40];
char port[6];
char id[120];
char user[40];
char pw[60];

char topic_prefix[] = "steaph/environments";

WiFiClient esp;
PubSubClient client(esp);


/*====================================================================
 *                    Arduino default functions
 *==================================================================== 
 */
void setup() {
  Serial.begin(115200);
  delay(10);
  
  pinMode(lampPin, OUTPUT); 
  pinMode(irSenderPin, OUTPUT);
  pinMode(irReceiverPin, INPUT);
  pinMode(pirPin, INPUT);
  pinMode(gasPin, OUTPUT);
  pinMode(tempPin, OUTPUT);

  delay(20);
  setStatus(false);
  digitalWrite(tempPin, LOW);
  digitalWrite(gasPin, LOW);

  connectWifi();
  configMqtt();
}

void loop() {
  connectBroker();
  publish();
  
  client.loop();
  Serial.println();
  delay(1000); // 10000 In production
}

/*====================================================================
 *                               WiFi
 *==================================================================== 
 */

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void connectWifi()
{
  if (WiFi.status() == WL_CONNECTED)
        return;
 
  delay(20);

  Serial.println("\n> Monting the FS...");

  if (SPIFFS.begin()) {
    Serial.println("> Mounted File system");
    if (SPIFFS.exists("/config.json")) {
      //SPIFFS.remove("/config.json");
      //file exists, reading and loading
      Serial.println("> Reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("> Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\n> Config parsed to json");

          strcpy(id, json["id"]);
          strcpy(broker, json["broker"]);
          strcpy(port, json["port"]);
          strcpy(user, json["user"]);
          strcpy(pw, json["pw"]);
        } else {
          Serial.println("\n\nERR> Failed to load json config");
        }
      }
    }
  } else {
    Serial.println("\n\nERR> Failed to mount FS");
  }

  WiFiManagerParameter custom_id("id", "Module Id", id, 100);
  WiFiManagerParameter custom_broker("broker", "Broker", broker, 40);
  WiFiManagerParameter custom_port("port", "Port", port, 6);
  WiFiManagerParameter custom_user("user", "User", user, 40);
  WiFiManagerParameter custom_pw("password", "Password", pw, 60);

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_id);
  wifiManager.addParameter(&custom_broker);
  wifiManager.addParameter(&custom_port);
  wifiManager.addParameter(&custom_user);
  wifiManager.addParameter(&custom_pw);
  
  if (!wifiManager.autoConnect("SteaphModule", "1234567890n")) {
    Serial.println("\n\nERR> Failed to connect and hit timeout");
    Serial.println("\n\nERR> Reseting...");
    delay(3000);
    
    ESP.reset();
    delay(5000);
  }

  strcpy(id, custom_id.getValue());
  strcpy(broker, custom_broker.getValue());
  strcpy(port, custom_port.getValue());
  strcpy(user, custom_user.getValue());
  strcpy(pw, custom_pw.getValue());

  if (shouldSaveConfig) {
    Serial.println("\n> Saving the configurations!");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["id"] = id;
    json["broker"] = broker;
    json["port"] = port;
    json["user"] = user;
    json["pw"] = pw;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("\n\nERR> Failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }

  Serial.println("");
  Serial.println("> WiFi connected");  
  Serial.println("> IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

/*====================================================================
 *                                MQTT
 *==================================================================== 
 */

// ###########
// MQTT CONFIG
// ###########
void configMqtt()
{
  client.setServer(broker, atoi(port));
  client.setCallback(mqtt_callback);
}

void connectBroker()
{
  connectWifi();
  
  while (!client.connected()) 
  {
    Serial.print("\n> Trying to connect to Broker:");
    Serial.println(broker);
    if (client.connect(id, user, pw)) 
    {
        Serial.println("> Succefuly connected to the Broker!");
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

void mqtt_callback(char* t, byte* payload, unsigned int length) 
{
    String msg;
    for(int i = 0; i < length; i++)
       msg += (char)payload[i];
       
    if(String(topic("status")).equals(String(t))){
      setStatus(msg != "false");
    }
}

// ##################
// MQTT COMMUNICATION
// ##################
void publish()
{
  float t = temperature();
  Serial.println(String("> Publishing the temperature: ") + String(t) + "˚C");
  client.publish(topic("temp"), String(t).c_str());

  const char* p = pir() ? "true" : "false";
  Serial.println(String("> Publishing the motion: ") + p);
  client.publish(topic("pir"), p);

  float g = gas();
  Serial.println(String("> Publishing the gas: ") + String(g));
  client.publish(topic("gas"), String(g).c_str());
}

void subscribe()
{
  Serial.println("\n> Subscribing the status...\n");
  client.subscribe(topic("status"), 0);
}

const char* topic(String suffix) {
  String buff = String(topic_prefix) + "/" + String(id) + "/" + suffix;
  return buff.c_str();
}

/*====================================================================
 *                      Sensors Configuration
 *==================================================================== 
 */

// ####################################
// GIVE IN REAL TIME THE TEMPREATURE °C
// ####################################
float temperature()
{
  digitalWrite(tempPin, HIGH);
  delay(10);
  
  float temp = (analogRead(A0) * 330.0f) / 1023.0f;
  digitalWrite(tempPin, LOW);
  
  return temp;
}

bool pir() 
{
  return digitalRead(pirPin) == HIGH;
}

float gas()
{
  digitalWrite(gasPin, HIGH);
  delay(500);
  
  float gas = analogRead(A0);
  digitalWrite(gasPin, LOW);
  
  return gas;
}

/*====================================================================
 *                            Actuador
 *==================================================================== 
 */

// #############################
// CHANGE THE ENVIRONMENT STATUS
// #############################
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
