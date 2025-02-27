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
const char* serverUrl = "wss://dstethoscopecore.onrender.com";  // ✅ ใช้ `wss://`
const int serverPort = 443;  // ✅ ใช้พอร์ต `443`

// ✅ ตั้งค่า WebSocket Client
WebSocketsClient webSocket;

// ✅ ตั้งค่า OLED Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* SCL=*/ 41, /* SDA=*/ 40, /* RST=*/ U8X8_PIN_NONE);

// ✅ ตั้งค่า I2S สำหรับไมโครโฟน
#define I2S_WS  15  // LRCL
#define I2S_SD  13  // DOUT
#define I2S_SCK 14  // BCLK
#define SAMPLE_RATE 16000  // ✅ Sample Rate 16 kHz
#define BUFFER_SIZE 1024  // ✅ ขนาดบัฟเฟอร์เก็บข้อมูลเสียง

int16_t i2sBuffer[BUFFER_SIZE];  // ✅ บัฟเฟอร์เก็บข้อมูลเสียงจาก I2S

// ✅ ฟังก์ชันตั้งค่า I2S
void setupI2S() {
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),  // ✅ ตั้งค่าเป็น Receiver (รับเสียง)
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false
    };

    i2s_pin_config_t pinConfig = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pinConfig);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

// ✅ ฟังก์ชันอ่านเสียงจาก I2S
String readAudioData() {
    size_t bytesRead;
    i2s_read(I2S_NUM_0, (void*)i2sBuffer, BUFFER_SIZE * sizeof(int16_t), &bytesRead, portMAX_DELAY);

    // ✅ แปลงเป็น Base64
    return base64::encode((uint8_t*)i2sBuffer, bytesRead);
}

// ✅ ฟังก์ชันแสดงข้อความบน OLED
void displayMessage(const char* line1, const char* line2) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 20, line1);
    u8g2.drawStr(10, 40, line2);
    u8g2.sendBuffer();
}

// ✅ ฟังก์ชันจัดการ WebSocket Event
void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("⚠️ WebSocket Disconnected! Reconnecting...");
            displayMessage("WebSocket", "Disconnected!");
            break;
        case WStype_CONNECTED:
            Serial.println("✅ WebSocket Connected!");
            displayMessage("WebSocket", "Connected!");
            break;
        case WStype_TEXT:
            Serial.printf("📩 Received message: %s\n", payload);
            break;
    }
}

// ✅ ฟังก์ชันเชื่อมต่อ WebSocket
void setupWebSocket() {
    webSocket.beginSSL("dstethoscopecore.onrender.com", serverPort, "/");  // ✅ ใช้ `beginSSL()` สำหรับ `wss://`
    webSocket.onEvent(onWebSocketEvent);
    webSocket.setReconnectInterval(5000);  // ✅ ลอง reconnect ทุก 5 วินาที
    webSocket.enableHeartbeat(10000, 3000, 2);  // ✅ ส่ง ping ทุก 10 วิ
}

// ✅ ฟังก์ชันเชื่อมต่อ WiFi
void connectWiFi() {
    displayMessage("Connecting", "WiFi...");
    WiFi.begin(ssid, password);
    int retryCount = 0;

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        retryCount++;
        if (retryCount > 20) {  // ✅ ถ้าเกิน 10 วินาทีให้ Restart ESP
            Serial.println("❌ WiFi Failed! Restarting...");
            ESP.restart();
        }
    }

    Serial.println("\n✅ WiFi Connected!");
    displayMessage("WiFi Connected", WiFi.localIP().toString().c_str());
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    Wire.begin();
    u8g2.begin();
    displayMessage("Initializing", "Please wait...");

    connectWiFi();
    setupI2S();  // ✅ ตั้งค่า I2S
    setupWebSocket();
}

void loop() {
    webSocket.loop();  // ✅ จัดการ WebSocket Connection

    // ✅ อ่านเสียงจากไมโครโฟน
    String audioBase64 = readAudioData();

    // ✅ สร้าง JSON Packet
    StaticJsonDocument<512> jsonDoc;
    jsonDoc["audio"] = audioBase64;
    String jsonStr;
    serializeJson(jsonDoc, jsonStr);

    // ✅ ส่งข้อมูลไปยัง WebSocket Server
    webSocket.sendTXT(jsonStr);

    delay(100);  // ✅ ส่งเสียงทุก 100ms
}
