#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "base64.h"
#include <driver/i2s.h>
#include <U8g2lib.h>
#include <Wire.h>  // ✅ I2C สำหรับ OLED

// ✅ ตั้งค่า WiFi
const char* ssid = "TEST";
const char* password = "6519g10001";
const char* serverUrl = "wss://dstethoscopecore.onrender.com";  // ✅ ใช้ WebSocket Secure
const int serverPort = 5000;

// ✅ ตั้งค่า I2S
#define I2S_WS  21
#define I2S_SD  47
#define I2S_SCK 48
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 512  // ✅ ลดขนาด Buffer ให้เหมาะสม

// ✅ ตั้งค่า OLED Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* SCL=*/ 41, /* SDA=*/ 40, /* RST=*/ U8X8_PIN_NONE);

// ✅ WebSocket Client
WebSocketsClient webSocket;

// ✅ แสดงข้อความบน OLED
void displayMessage(const char* line1, const char* line2) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 20, line1);
    u8g2.drawStr(10, 40, line2);
    u8g2.sendBuffer();
}

// ✅ ตั้งค่า I2S
void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_SIZE,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ✅ จัดการ WebSocket Event
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("❌ WebSocket Disconnected!");
            displayMessage("WebSocket", "Disconnected!");
            break;

        case WStype_CONNECTED:
            Serial.println("✅ WebSocket Connected!");
            displayMessage("WebSocket", "Connected!");
            break;

        case WStype_TEXT:
            Serial.printf("📥 AI Response: %s\n", payload);
            displayMessage("AI Response:", (char*)payload);
            break;
    }
}

// ✅ เชื่อมต่อ WebSocket
void setupWebSocket() {
    webSocket.begin(serverUrl, serverPort, "/ws");
    webSocket.onEvent(webSocketEvent);
}

// ✅ เชื่อมต่อ WiFi
void connectWiFi() {
    displayMessage("Connecting", "WiFi...");
    WiFi.begin(ssid, password);
    int retryCount = 0;

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        retryCount++;
        if (retryCount > 20) {  // ลอง 10 วินาทีแล้ว Restart
            ESP.restart();
        }
    }

    Serial.println("\n✅ WiFi Connected!");
    displayMessage("WiFi Connected", WiFi.localIP().toString().c_str());
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    Wire.begin();  // ✅ ตั้งค่า I2C
    u8g2.begin();
    displayMessage("Initializing", "Please wait...");

    setupI2S();
    connectWiFi();
    setupWebSocket();
}

void loop() {
    int16_t buffer[BUFFER_SIZE];
    size_t bytes_read;
    i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

    if (bytes_read > 0) {
        Serial.println("🎤 Captured Audio Data");

        // ✅ แปลงเป็น Base64
        String encodedAudio = base64::encode((uint8_t*)buffer, bytes_read);

        // ✅ สร้าง JSON และส่งผ่าน WebSocket
        StaticJsonDocument<1024> jsonDoc;
        jsonDoc["audio"] = encodedAudio;

        String jsonString;
        serializeJson(jsonDoc, jsonString);

        webSocket.sendTXT(jsonString);
        displayMessage("Sending Audio", "To Server...");
    }

    webSocket.loop();
}
