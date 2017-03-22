#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

//! Log a message
#define LOG_INFO(str) \
   do { Serial.println(str); } while(0)

//! Log a formatted message
#define LOG_INFO_F(str, __ARGS__) \
   do { Serial.printf(str, __ARGS__); } while(0)

const size_t MQTT_SERVER_NAME_SIZE = 40;
const size_t MQTT_PORT_NAME_SZIE = 6;

char mqtt_server[MQTT_SERVER_NAME_SIZE] = {0};
char mqtt_port[MQTT_PORT_NAME_SZIE] = "1883";

//! Name of the Soft AP for the case then connection
//! failed
const char * const soft_ap_name = "ESP-PC";

//! Password of the Soft AP
const char * const soft_ap_password = "password";

const uint8_t PC_STATUS_PIN = 4;
const uint8_t PC_SHUTDOWN_PIN = 5;

//! Current PC status @b true for ON @b false for OFF
bool pcStatus = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
   Serial.begin(115200);

   // Read PC status from LED+ pin
   pinMode(PC_STATUS_PIN, INPUT);

   // Power switch - drive pin HIGH to "click" the power button
   pinMode(PC_SHUTDOWN_PIN, OUTPUT);
   digitalWrite(PC_SHUTDOWN_PIN, LOW);

   // By default reset the WiFi Manager config. Keep it only on successful
   // configuration read
   bool resetConfig = true;

   LOG_INFO("Mounting FS");
   if (SPIFFS.begin()) {
      LOG_INFO("Mounted file system");

      if (SPIFFS.exists("/config.json")) {
         LOG_INFO("Reading config file");

         File configFile = SPIFFS.open("/config.json", "r");
         if (configFile) {
            LOG_INFO("Opened config file");

            size_t size = configFile.size();

            std::unique_ptr<char[]> buf(new char[size]);
            configFile.readBytes(buf.get(), size);

            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(buf.get());

            json.printTo(Serial);
            if (json.success()) {
               LOG_INFO("Parsed json");

               strcpy(mqtt_server, json["mqtt_server"]);
               strcpy(mqtt_port, json["mqtt_port"]);

               resetConfig = false;
            } else {
               LOG_INFO("Failed to load json config");
            }
         }
      }
   } else {
      LOG_INFO("Failed to mount FS");
   }

   // Extra parameters
   WiFiManagerParameter custom_mqtt_server("server", "mqtt server",
      mqtt_server, MQTT_SERVER_NAME_SIZE - 1);

   WiFiManagerParameter custom_mqtt_port("port", "mqtt port",
      mqtt_port, MQTT_PORT_NAME_SZIE - 1);

   static bool shouldSaveConfig = false;

   WiFiManager wifiManager;
   wifiManager.setSaveConfigCallback([](){
      LOG_INFO("Should save config");
      shouldSaveConfig = true;
   });

   // Add extra parameters
   wifiManager.addParameter(&custom_mqtt_server);
   wifiManager.addParameter(&custom_mqtt_port);

   // Reset settings - seems like without executing this line at least once the
   // module won't start properly and will report successful connection.
   if (resetConfig) {
      wifiManager.resetSettings();
   }

   // Set default minimal quality of signal so it ignores AP's under that quality (8%)
   wifiManager.setMinimumSignalQuality();

   // Fetch SSID and pass and try to connect.
   // Starts an access point with the specified name (i.e. soft_ap_name)
   // in case of connection failure.
   if (!wifiManager.autoConnect(soft_ap_name, soft_ap_password)) {
      LOG_INFO("Failed to connect and hit timeout");
      delay(3000);

      // Reset and try again
      ESP.reset();
      delay(5000);
   }

   // At this point we have a connection to our AP
   LOG_INFO("Connected to AP");

   // Read updated parameters
   strcpy(mqtt_server, custom_mqtt_server.getValue());
   strcpy(mqtt_port, custom_mqtt_port.getValue());

   client.setServer(mqtt_server, atoi(mqtt_port));
   client.setCallback([](char *topic, byte *payload, unsigned int length){
      // Print out the message
      LOG_INFO_F("Message arrived [%s]", topic);

      // We are not interested in message itself it can only be "on" or "off"
      // either way the only thing we can do is simulate a 500 ms power button click
      digitalWrite(PC_SHUTDOWN_PIN, HIGH);
      delay(500);
      digitalWrite(PC_SHUTDOWN_PIN, LOW);
   });

   // Save the custom parameters to FS
   if (shouldSaveConfig) {
      LOG_INFO("Saving config");

      DynamicJsonBuffer jsonBuffer;
      JsonObject &json = jsonBuffer.createObject();
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;

      File configFile = SPIFFS.open("/config.json", "w");
      if (configFile) {
         json.prettyPrintTo(Serial);
         json.printTo(configFile);
         configFile.close();
      } else {
         LOG_INFO("Failed to open config file for writing");
      }
   }
}

/**
 * Tries to connect to the MQTT server
 */
void reconnect() {
   // Loop until we're reconnected
   while (!client.connected()) {
      LOG_INFO("Attempting MQTT connection...");
      if (client.connect(soft_ap_name)) {
         LOG_INFO("MQTT Client connected");
         client.subscribe("pc/switch");
      } else {
         LOG_INFO_F("Connection failed, rc=%d trying again in 5 seconds",
                    client.state());
         delay(5000);
      }
   }
}

/**
 * Main loop
 */
void loop() {
   // Ensure we are connected to the MQTT server
   if (!client.connected()) {
      reconnect();
   }
   client.loop();

   // Publish only if pin readings are changed
   bool newStatus = (digitalRead(PC_STATUS_PIN) != LOW);
   if (pcStatus != newStatus) {
      client.publish("pc/status", (newStatus ? "on" : "off"), true);
      pcStatus = newStatus;
   }
}
