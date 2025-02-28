#include <WiFi.h>
#include <PubSubClient.h>
#include <driver/i2s.h>
#include "arduinoFFT.h"
#include <Wire.h>
#include <U8g2lib.h>

// **ตั้งค่าจอ OLED 0.9 นิ้ว (I2C)**
#define OLED_SDA 40
#define OLED_SCL 41
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0, 
    /* clock=*/ OLED_SCL, 
    /* data=*/ OLED_SDA, 
    /* reset=*/ U8X8_PIN_NONE  // เปลี่ยนตำแหน่ง reset ไปอยู่ท้ายสุด
);
// **ตั้งค่าขา I2S สำหรับ INMP441**
#define I2S_WS  21
#define I2S_SCK 48
#define I2S_SD  47

#define FFT_SIZE 1024
#define SAMPLE_RATE 16000

// **ตั้งค่า MQTT**
const char* mqtt_server = "your-mqtt-broker.com";
const char* mqtt_topic = "lung/mfcc";

WiFiClient espClient;
PubSubClient client(espClient);

// ** แก้ไขการประกาศ ArduinoFFT **
double real[FFT_SIZE];
double imag[FFT_SIZE];
ArduinoFFT<double> FFT = ArduinoFFT<double>(real, imag, FFT_SIZE, SAMPLE_RATE);

// **ฟังก์ชันแสดงสถานะบน OLED**
void updateOLED(String status) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.setCursor(0, 10);
    u8g2.print("StethoMicore AI");

    u8g2.setCursor(0, 25);
    u8g2.print("Status: ");
    u8g2.print(status);

    u8g2.setCursor(0, 40);
    u8g2.print("WiFi: ");
    u8g2.print(WiFi.status() == WL_CONNECTED ? "Connected" : "Connecting...");

    u8g2.setCursor(0, 55);
    u8g2.print("MQTT: ");
    u8g2.print(client.connected() ? "Connected" : "Disconnected");
    
    u8g2.sendBuffer();
}

// **ฟังก์ชันเชื่อมต่อ WiFi**
void setup_wifi() {
    WiFi.begin("TEST", "6519g10001");
    updateOLED("Connecting to WiFi...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected.");
    updateOLED("WiFi Connected!");
}

// **ตั้งค่า I2S รับเสียงจาก INMP441**
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
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

// **อ่านเสียง, คำนวณ FFT และส่งข้อมูลให้ AI**
void compute_fft_and_send() {
    int16_t buffer[FFT_SIZE];
    size_t bytes_read;
    
    // อ่านข้อมูลเสียงจาก I2S
    esp_err_t result = i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
    if (result != ESP_OK || bytes_read == 0) {
        Serial.println("I2S Read Failed!");
        updateOLED("I2S Read Error!");
        return;
    }

    // แปลง PCM เป็น float และใช้ Log Scaling
    for (int i = 0; i < FFT_SIZE; i++) {
       real[i] = log10((double)abs(buffer[i]) + 1); // แปลง buffer[i] เป็น double ก่อนใช้ log10()
        imag[i] = 0;
    }

    // **แก้ไข: เปลี่ยนเป็น FFT_WIN_TYP_HAMMING**
    FFT.windowing(real, FFT_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(real, imag, FFT_SIZE, FFT_FORWARD);
    FFT.complexToMagnitude(real, imag, FFT_SIZE);

    // ใช้ G.711 บีบอัดข้อมูลก่อนส่งให้ AI
    uint8_t compressed_fft[FFT_SIZE / 2];  
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        compressed_fft[i] = real[i] > 0 ? log10((double)real[i]) * 10 : 0; // แปลง real[i] เป็น double
    }
    client.publish(mqtt_topic, compressed_fft, sizeof(compressed_fft));

    updateOLED("FFT & AI Processing...");
}

// **ฟังก์ชันเชื่อมต่อ MQTT**
void reconnect_mqtt() {
    while (!client.connected()) {
        updateOLED("Connecting to MQTT...");
        Serial.println("MQTT reconnecting...");
        if (client.connect("ESP32_Client")) {
            updateOLED("MQTT Connected!");
        } else {
            delay(5000);
        }    }
}

// **แสดงสถานะแบตเตอรี่บน OLED (ตัวอย่าง)**
void showBatteryStatus(int percentage) {
    u8g2.setCursor(100, 10);
    u8g2.print(percentage);
    u8g2.print("%");
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);

    // ตั้งค่าหน้าจอ OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    
    updateOLED("Initializing...");

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    setup_i2s();
}

void loop() {
    if (!client.connected()) {
        reconnect_mqtt();
    }
    client.loop();
    compute_fft_and_send();

    // แสดงแบตเตอรี่ 85% (ตัวอย่าง)
    showBatteryStatus(85);
}
