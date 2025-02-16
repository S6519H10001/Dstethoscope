#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "DHT.h"
#include <ArduinoJson.h>

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO


Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define DHTPIN 4 
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//MIC_PIN
#define MIC_PIN 4  // ADC1 channel
const int sampleSize = 512;  // Adjust based on needs
float amplitude[sampleSize];
float timeValues[sampleSize];
String jsonString;
    
// Wi-Fi credentials
const char* ssid = "TEST";
const char* password = "1234";

// MQTT Broker details
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int mqtt_port = 1883;
const char* publish_topic ="test/audio/jsonStringTest";
const char* publish_topic_temp = "test/topicTar/temp";
const char* publish_topic_humi = "test/topicTar/humi";
const char* subscribe_topic = "python/testTar";
// Wi-Fi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  delay(10);
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi.");
    // Once connected, print IP address
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Print the ESP32's IP address
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      client.publish(publish_topic, jsonString.c_str());
      client.subscribe(subscribe_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  display.begin(i2c_Address, true); // Address 0x3C default
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Electronic ");
  display.setCursor(0, 10);
  display.println("stethoscope");
  display.display();

  delay(2000);

  dht.begin();
  
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

    for (int i = 0; i < sampleSize; i++) {
        timeValues[i] = i * 0.01; // Example: 10ms interval
    }
}

void loop() {

        for (int i = 0; i < sampleSize; i++) {
        int rawValue = analogRead(MIC_PIN);
        amplitude[i] = (rawValue / 4095.0) * 100.0; // Convert to range 0-100
        delayMicroseconds(62);  // ~16kHz sample rate
    }

    // Create JSON
    StaticJsonDocument<2048> jsonDoc;
    JsonArray amplitudeArray = jsonDoc.createNestedArray("amplitude");
    JsonArray timeArray = jsonDoc.createNestedArray("time");

    for (int i = 0; i < sampleSize; i++) {
        amplitudeArray.add(amplitude[i]);
        timeArray.add(timeValues[i]);
    }

    // Serialize and print JSON

    serializeJson(jsonDoc, jsonString);
    Serial.println(jsonString);

    // MQTT Reconnect
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();
    publishSensorData();
  // Example of publishing a message
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 1000) {
    lastTime = millis();
 
    int temp = dht.readTemperature();
    int humi = dht.readHumidity(); 
    
    // Convert JSON to string
    char jsonString[5];
    
    //serializeJson(jsonDoc, jsonString);
    //client.publish(publish_topic, jsonString);
    client.publish(publish_topic_temp,String(temp).c_str());
    client.publish(publish_topic_humi, String(humi).c_str());
    Serial.print("Message published.");
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Electronic ");
    display.setCursor(0, 10);
    display.println("stethoscope");
    display.display();
    delay(2000);
    display.clearDisplay();
  }
}
void publishSensorData() {
    StaticJsonDocument<1024> jsonDoc;
    JsonArray amplitudeArray = jsonDoc.createNestedArray("amplitude");
    JsonArray timeArray = jsonDoc.createNestedArray("time");

    // Your sample amplitude and time data
    float amplitude[] = { 
        0.0, 30.9017, 58.7785, 80.9017, 95.1057, 100.0
    };

    float time[] = { 
        0.0, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07
    };

    int dataSize = sizeof(amplitude) / sizeof(amplitude[0]); 

    for (int i = 0; i < dataSize; i++) {
        amplitudeArray.add(amplitude[i]);
        timeArray.add(time[i]);
    }

    // Convert JSON to string
    char jsonString[512];
    serializeJson(jsonDoc, jsonString);

    // Publish to MQTT topic
    client.publish(publish_topic, jsonString);
    Serial.println("MQTT Message Sent: ");
    Serial.println(jsonString);
}
