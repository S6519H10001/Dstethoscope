#include <WiFi.h>
#include <WebSocketsClient.h>
#include <driver/i2s.h>

#define I2S_WS 21    // LRCL
#define I2S_SD 47    // DOUT
#define I2S_SCK 48   // BCLK

#define SAMPLE_RATE 16000
#define I2S_BUFFER_SIZE 1024

const char* ssid = "TEST";
const char* password = "6519g10001";

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

void setup() {
    Serial.begin(115200);
    setupI2S();
    Serial.println("🎤 Ready to capture audio...");
}

void loop() {
    int16_t samples[I2S_BUFFER_SIZE];
    size_t bytesRead;

    i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, portMAX_DELAY);

    if (bytesRead > 0) {
        for (int i = 0; i < bytesRead / 2; i++) { // 16-bit (2 bytes per sample)
            Serial.println(samples[i]);  // ส่งข้อมูลเสียงไปยัง Serial Monitor
        }
    } else {
        Serial.println("⚠️ No data read from I2S");
    }

    delay(100);
}
