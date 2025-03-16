#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <U8g2lib.h>

const char* ssid = "TEST";
const char* password = "6519g10001";
const char* serverUrl = "ws://10.1.1.165:8000/audio";

using namespace websockets;
WebsocketsClient client;

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 1024  
#define AUDIO_GAIN 300  

int32_t audioBuffer[BUFFER_SIZE];  

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ 40, /* data=*/ 41, /* reset=*/ U8X8_PIN_NONE);

void displayStatus(const char* status) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 20, "Stethomicore");
    u8g2.drawStr(10, 10, status);
    u8g2.sendBuffer();
}

void setup_wifi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi!");
        return;
    }
    Serial.println("WiFi Connected!");
    displayStatus("WiFi Connected!");
}

void connectWebSocket() {
    Serial.println("Connecting to WebSocket Server...");
    displayStatus("Connecting WebSocket...");
    
    if (client.connect(serverUrl)) {
        Serial.println("Connected to WebSocket Server!");
        displayStatus("WebSocket Connected!");
    } else {
        Serial.println("WebSocket Connection Failed!");
        displayStatus("WebSocket Failed!");
    }
}

void setup_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 10,
        .dma_buf_len = 256
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

float bandpassFilter(float sample) {
    static float x[3] = {0, 0, 0};
    static float y[3] = {0, 0, 0};
    
    x[0] = x[1];
    x[1] = x[2];
    x[2] = sample;
    
    y[0] = y[1];
    y[1] = y[2];
    y[2] = (0.2066 * x[2]) - (0.4132 * x[1]) + (0.2066 * x[0]) +
           (1.2113 * y[1]) - (0.4965 * y[0]);

    return y[2];
}

#define FILTER_WINDOW 5
float movingAverageFilter(float sample) {
    static float buffer[FILTER_WINDOW] = {0};
    static int index = 0;
    static float sum = 0;

    sum -= buffer[index];
    buffer[index] = sample;
    sum += buffer[index];

    index = (index + 1) % FILTER_WINDOW;

    return sum / FILTER_WINDOW;
}

void sendAudioWebSockets() {
    size_t bytesRead = 0;
    i2s_read(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);

    if (bytesRead > 0) {
        int samplesRead = bytesRead / sizeof(int32_t);
        int16_t audioBuffer16[BUFFER_SIZE];

        for (int i = 0; i < samplesRead; i++) {
            int32_t sample32 = (audioBuffer[i] & 0xFFFFFF00) >> 8;
            int16_t sample16 = (int16_t)(sample32 >> 8); 
            sample16 = bandpassFilter(sample16);
            sample16 = movingAverageFilter(sample16);
            sample16 *= AUDIO_GAIN;
            audioBuffer16[i] = constrain(sample16, -32768, 32767);
        }

        if (!client.available()) {
            Serial.println("\u274C WebSocket Disconnected. Reconnecting...");
            connectWebSocket();
        } else {
            client.sendBinary((const char*)audioBuffer16, samplesRead * sizeof(int16_t));
            client.poll();
        }
    }
    delay(100);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(40, 41);
    u8g2.begin();
    displayStatus("Booting...");
    setup_wifi();
    setup_i2s();
    connectWebSocket();
}

void loop() {
    sendAudioWebSockets();
    delay(10);
}
