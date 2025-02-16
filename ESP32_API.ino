#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>


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


    
// Wi-Fi credentials
const char* ssid = "TES";
const char* password = "1234";
const char* serverUrl = "http://xx.xx.xx.xx:1880/audio"; // Replace with your server

// MQTT Broker details
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int mqtt_port = 1883;
String numbers[256];
const int numSamples = 1000;
float amplitude[numSamples];
float timeValues[numSamples];
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
  //WiFi.begin(ssid, password);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
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
      client.publish(publish_topic, "TEST");
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

    for (int i = 0; i < numSamples; i++) {
        timeValues[i] = i * 0.01;  // Example: 10ms interval
    }
}
#define TOPIC_COUNT 100
char mqttTopics[TOPIC_COUNT][20];  // Array to store topics

void generateTopics() {
    for (int i = 0; i < TOPIC_COUNT; i++) {
        snprintf(mqttTopics[i], sizeof(mqttTopics[i]), "esp32/sensor/%d", i);
    }
}
void publishSensorData() {
    // sampleSize = 10;  b'{"amplitude":[0,0,0,0,0,0,0,0,0,0],"time":[0,0.01,0.02,0.03,0.04,0.05,0.06,0.07,0.08,0.09]}' on topic test/audio/jsonStringTest  on len 91
    const int sampleSize = 10;  // Adjust based on needs
    float amplitude[sampleSize];
    float time[sampleSize];
    StaticJsonDocument<1024> jsonDoc;
    JsonArray amplitudeArray = jsonDoc.createNestedArray("amplitude");
    JsonArray timeArray = jsonDoc.createNestedArray("time");
    for (int i = 0; i < sampleSize; i++) {
        time[i] = i * 0.01; // Example: 10ms interval
    }

   for (int i = 0; i < sampleSize; i++) {
        int rawValue = analogRead(MIC_PIN);
        amplitude[i] = (rawValue / 4095.0) * 100.0; // Convert to range 0-100
        delayMicroseconds(62);  // ~16kHz sample rate
    }
  /*   
    // Your sample amplitude and time data
    float amplitude[] = { 
        0.0, 30.9017, 58.7785, 80.9017, 95.1057, 100.0
    };

    float time[] = { 
        0.0, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07
    };
   */
    int dataSize = sizeof(amplitude) / sizeof(amplitude[0]); 

    for (int i = 0; i < dataSize; i++) {
        amplitudeArray.add(amplitude[i]);
        timeArray.add(time[i]);
    }

    // Convert JSON to string
    char jsonString[512];
    serializeJson(jsonDoc, jsonString);
    /*
    // Generate topics
    generateTopics();
 
    // Print topics
    for (int i = 0; i < TOPIC_COUNT; i++) {
        Serial.println(mqttTopics[i]);
    }
     for (int i = 0; i < TOPIC_COUNT; i++) {
          client.publish(mqttTopics[i],jsonString);
      }
     */
    for (int i = 0; i < 20; i++) {  // Publish only 10 at a time (to avoid overload)
            char topic[30];
            char message[10];
            snprintf(topic, sizeof(topic), "esp32/sensor/%d", i);
            //snprintf(message, sizeof(message), "Data %d", i);

            client.publish(topic, jsonString);
            Serial.print("Published: ");
            Serial.print(topic);
            Serial.print(" -> ");
            Serial.println(jsonString);
        }
        
    Serial.print("MQTT Message Sent: ");
    Serial.println(jsonString);
    
}
void api()
{
    const int sampleSize = 1024;  // Adjust based on needs
  uint16_t audioData[sampleSize];

  // Read ADC values
  for (int i = 0; i < sampleSize; i++) {
    audioData[i] = analogRead(MIC_PIN);
    delayMicroseconds(62);  // ~16kHz sample rate
  }

  // Send data to server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");

    int httpResponseCode = http.POST((uint8_t*)audioData, sizeof(audioData));
    //  int httpCode = http.GET();
    if (httpResponseCode == 200) {
      String content = http.getString();
      Serial.println("Content ---------");
      Serial.println(content);
      Serial.println("-----------------");
    } else {
      Serial.println("Fail. error code " + String(httpResponseCode));
    }
    Serial.println("END POST");
    Serial.println("Response: " + String(httpResponseCode));

  }

  delay(100);  // Adjust based on transmission needs
}
void sendSineWaveData(float amplitude) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{\"amplitude\": " + String(amplitude) + "}";
        
        int httpResponseCode = http.POST(jsonPayload);
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Server Response: " + response);
        } else {
            Serial.print("Error in POST request: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi Disconnected!");
    }
}
void loop() {
  float amplitude = 100 * sin(2 * PI * millis() / 1000.0);  // Generate sine wave
    Serial.println("Sending amplitude: " + String(amplitude));
    
    //sendSineWaveData(amplitude);
  api();
 /*
    // MQTT Reconnect
    if (!client.connected()) {
      reconnectMQTT();
    }
  client.loop();
  // publishSensorData();
  //  sampling_MQTT();    
  // Example of publishing a message
  */
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
    display.setCursor(0, 40);
    display.println(    WiFi.localIP());

    display.display();
    delay(2000);
    display.clearDisplay();
  }
}
