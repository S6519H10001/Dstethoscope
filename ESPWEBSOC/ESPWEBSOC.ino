#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>

const char* ssid = "TEST";
const char* password = "6519g10001";
const char* serverUrl = "ws://your-server.com:8080";  // ใช้ WebSockets Server

using namespace websockets;
WebsocketsClient client;

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 1024  // ขนาด Buffer 1024 ตัวอย่างต่อแพ็กเกจ

int16_t audioBuffer[BUFFER_SIZE];

void setup_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected!");
}

void setup_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .bck_io_num = 48,
        .ws_io_num = 21,
        .data_out_num = -1,
        .data_in_num = 47
    };
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void sendAudioWebSockets() {
    size_t bytesRead;
    i2s_read(I2S_NUM_0, &audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);

    if (bytesRead > 0 && client.available()) {
        client.sendBinary((const char*)audioBuffer, bytesRead);
        Serial.println("Audio data sent via WebSockets!");
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    setup_i2s();

    if (client.connect(serverUrl)) {
        Serial.println("Connected to WebSocket Server!");
    } else {
        Serial.println("Failed to connect.");
    }
}

void loop() {
    sendAudioWebSockets();
    delay(100);  // ส่งข้อมูลทุก 100ms (Real-time)
}
