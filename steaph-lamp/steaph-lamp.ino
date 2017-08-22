#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/*====================================================================
 *                         I/O Config
 *==================================================================== 
 */
#define lampPin 0

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

  delay(20);
  setStatus(false);

  connectWifi();
  configMqtt();
}

void loop() {
  connectBroker();
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
    return;
  }

  Serial.println("> Environment disable!");
  digitalWrite(lampPin, HIGH); // Inverse
 }

