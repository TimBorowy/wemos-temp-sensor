
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <DHT.h>

#include "Settings.h"

// Port where DTH22 is connected (D4 is IO2 on wemos D1 mini)
#define DHTPIN 2
 // I use DHT22, change to other DHT sensor if you use other one
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = CONFIG_SSID;
const char* password = CONFIG_PASS;
const char* deviceId = CONFIG_DEVICE_ID;
const char* mqtt_server = CONFIG_MQTT_SERVER;
const char* mqtt_user = CONFIG_MQTT_USER;
const char* mqtt_pass = CONFIG_MQTT_PASS;

WiFiClient espClient;
PubSubClient mqtt(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();

  mqtt.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  WiFiManager wm;
  // Resets WiFi setting for development
  //wm.resetSettings();

  bool res;
  res = wm.autoConnect("Temp Sensor setup AP");

  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } else {
    // If you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
  }
}

void sendTemperature() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  } else {
    sendMqttRequest(h, t);
  }
}

void sendMqttRequest(float h, float t) {
  const size_t capacity = JSON_OBJECT_SIZE(6);
  DynamicJsonDocument doc(capacity);

  char values[16];
  sprintf(values,"%g;%g;0", t, h);
  // Char to string
  String sensorValueString = String(values);
  //Serial.println(sensorValueString);

  doc["command"] = "udevice";
  doc["idx"] = 28;
  doc["signal"] = WiFi.RSSI();
  doc["device_id"] = deviceId;
  doc["svalue"] = sensorValueString;

  char buffer[capacity];
  serializeJson(doc, buffer);
  
  Serial.println(buffer);
  mqtt.publish("temp", buffer);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(deviceId, mqtt_user, mqtt_pass)){
      Serial.println("connected");
    } else {
      Serial.print("failed to connect to: ");
      Serial.print(mqtt_server);
      Serial.print(" Using user/pass: ");
      Serial.print(mqtt_user);
      Serial.print(" / ");
      Serial.print(mqtt_pass);
      Serial.print("rc=");
      Serial.print(mqtt.state());
      
      Serial.println(" trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  // MQTT incoming msg loop
  mqtt.loop();
  delay(10000);
  sendTemperature();
}
