#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <U8g2lib.h>

// ✅ ตั้งค่า OLED Display (0.9 นิ้ว, SSD1306, 128x64)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* SCL=*/ 41, /* SDA=*/ 40, /* RST=*/ U8X8_PIN_NONE);

// ✅ ตั้งค่าตัวแปรพื้นฐาน
#define SAMPLE_RATE 16000  
#define I2S_PORT I2S_NUM_0
#define BUFFER_SIZE 1024  
const int sampleSize = 1024;  

// ✅ ตั้งค่า I2S ไมโครโฟน (INMP441)
#define I2S_WS  21   
#define I2S_SD  47   
#define I2S_SCK 48   
float amplitude[sampleSize];

// ✅ ตั้งค่า High-Pass Filter
float alpha = 0.9;  
int16_t lastSample = 0;

// ✅ ตั้งค่า Wi-Fi และ MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* publish_topic = "stethoscope/audio";
const char* ssid = "TEST";
const char* password = "6519g10001";

WiFiClientSecure wifiClient;  // ✅ ใช้ WiFiClientSecure
PubSubClient client(wifiClient);  // ✅ แก้จาก espClient เป็น wifiClient

// ✅ ฟังก์ชันแสดงข้อความบน OLED
void displayMessage(const char* line1, const char* line2) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 20, line1);
    u8g2.drawStr(10, 40, line2);
    u8g2.sendBuffer();
}

// ✅ ฟังก์ชันเชื่อมต่อ Wi-Fi
void setupWiFi() {
    displayMessage("Wi-Fi", "Connecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi!");
    displayMessage("Wi-Fi Connected", WiFi.localIP().toString().c_str());

    wifiClient.setInsecure();  // ✅ ปิดการตรวจสอบ SSL ทำแค่ครั้งเดียว
}

// ✅ ฟังก์ชันเชื่อมต่อ MQTT
void reconnectMQTT() {
    displayMessage("MQTT", "Connecting...");
    while (!client.connected()) {
        Serial.println("Reconnecting to MQTT...");
        if (client.connect("ESP32Client")) {
            displayMessage("MQTT Connected", "Ready to send data");
        } else {
            displayMessage("MQTT Failed", "Retrying...");
            delay(5000);
        }
    }
}

// ✅ ฟังก์ชันตั้งค่า I2S
void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1, 
        .data_in_num = I2S_SD
    };
    
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.println("Failed to install I2S driver!");
    }
    i2s_set_pin(I2S_PORT, &pin_config);
}

// ✅ ฟังก์ชันประมวลผลเสียง
void processAudio(int16_t *buffer, int sampleCount) {
    float maxAmplitude = 0;
    
    for (int i = 0; i < sampleCount; i++) {
        float filteredSample = alpha * (lastSample + buffer[i] - lastSample);
        lastSample = buffer[i];

        if (abs(filteredSample) > maxAmplitude) {
            maxAmplitude = abs(filteredSample);
        }

        amplitude[i] = filteredSample;
    }

    if (maxAmplitude > 0) {
        for (int i = 0; i < sampleCount; i++) {
            amplitude[i] = (amplitude[i] / maxAmplitude) * 32767.0;
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(40, 41);
    u8g2.begin();
    displayMessage("Initializing", "Please wait...");
    
    setupWiFi();
    setupI2S();
    client.setServer(mqtt_server, mqtt_port);
}

void loop() {  
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    displayMessage("Recording", "Listening...");

    int16_t buffer[1024]; 
    size_t bytes_read;
    i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
    int sampleCount = bytes_read / sizeof(int16_t);

    if (sampleCount <= 0) {
        displayMessage("Error", "No Audio Data");
        return;
    }

    processAudio(buffer, sampleCount);

    StaticJsonDocument<2048> jsonDoc;
    JsonArray amplitudeArray = jsonDoc.createNestedArray("amplitude");

    for (int i = 0; i < sampleCount; i += 4) { 
        amplitudeArray.add((int16_t) amplitude[i]);
    }

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        Serial.println("🔵 Sending audio data to API...");

        http.begin(wifiClient, "https://dstethoscopecore.onrender.com/audio");  // ✅ ใช้ wifiClient ที่ประกาศแล้ว
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(jsonString);

        Serial.print("🟢 HTTP Response code: ");
        Serial.println(httpResponseCode);

        http.end();
    } else {
        Serial.println("❌ Wi-Fi Disconnected!");
    }

    delay(5000);
}
