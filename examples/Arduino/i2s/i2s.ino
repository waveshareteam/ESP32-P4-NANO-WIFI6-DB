/*
 * ES8311 microphone-to-speaker loopback.
 *
 * The board pin and power setup follows the ESP32-P4-NANO-WIFI6-DB ESP-IDF BSP.
 * The microphone gain is configured locally for this Arduino example.
 */

#include <Arduino.h>
#include <ESP_I2S.h>
#include <Wire.h>

#include "ES8311.h"

constexpr uint32_t i2c_frequency_hz = 400000;
constexpr uint32_t sample_rate_hz = 48000;
constexpr size_t audio_buffer_size = 1024;

I2SClass i2s;
ES8311 codec;
int16_t audio_buffer[audio_buffer_size / sizeof(int16_t)];
uint32_t last_audio_log_ms = 0;

void scan_i2c_bus() {
  uint8_t device_count = 0;
  for (uint8_t address = 1; address < 0x7F; ++address) {
    Wire1.beginTransmission(address);
    if (Wire1.endTransmission() == 0) {
      Serial.printf("I2C device found at 0x%02X\n", address);
      ++device_count;
    }
  }
  Serial.printf("I2C scan complete: %u device(s)\n", device_count);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting ES8311 I2S loopback");

  if (!Wire1.begin(SDA, SCL, i2c_frequency_hz)) {
    Serial.println("Failed to initialize I2C");
    return;
  }
  scan_i2c_bus();

  i2s.setPins(I2S_BCLK, I2S_LRCLK, I2S_DOUT, I2S_DIN, I2S_MCLK);
  if (!i2s.begin(I2S_MODE_STD, sample_rate_hz, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    Serial.println("Failed to initialize I2S");
    return;
  }

  if (!codec.initialize(Wire1, PA_POWER, sample_rate_hz)) {
    Serial.println("Failed to initialize ES8311 at I2C address 0x18");
    i2s.end();
    return;
  }

  Serial.println("ES8311 ready: 48000 Hz, 16-bit mono, MCLK on GPIO13");
  Serial.println("ES8311 microphone gain: 24 dB");
  Serial.println("Microphone audio is now sent to the speaker. Avoid acoustic feedback.");
}

void loop() {
  size_t bytes_read = i2s.readBytes(reinterpret_cast<char *>(audio_buffer), sizeof(audio_buffer));
  if (bytes_read == 0) {
    return;
  }

  int32_t input_peak = 0;
  const size_t sample_count = bytes_read / sizeof(audio_buffer[0]);
  for (size_t index = 0; index < sample_count; ++index) {
    const int32_t sample = audio_buffer[index];
    const int32_t sample_level = sample < 0 ? -sample : sample;
    if (sample_level > input_peak) {
      input_peak = sample_level;
    }
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_audio_log_ms >= 1000) {
    Serial.printf("I2S input peak: %ld\n", static_cast<long>(input_peak));
    last_audio_log_ms = now_ms;
  }

  size_t bytes_written = i2s.write(reinterpret_cast<const uint8_t *>(audio_buffer), bytes_read);
  if (bytes_written != bytes_read) {
    Serial.printf("I2S short write: %u/%u bytes\n", static_cast<unsigned>(bytes_written), static_cast<unsigned>(bytes_read));
  }
}
