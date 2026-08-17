/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal Arduino ES8311 control helper for ESP32-P4-NANO-WIFI6-DB.
 * The register setup follows Espressif's esp_codec_dev ES8311 driver.
 */

#include "ES8311.h"

namespace {

struct register_value {
  uint8_t reg;
  uint8_t value;
};

constexpr uint8_t input_gain_24db_code = 0x04;

constexpr register_value open_sequence[] = {
    {0x0D, 0xFA},
    {0x44, 0x08},
    {0x44, 0x08},
    {0x01, 0x30},
    {0x02, 0x00},
    {0x03, 0x10},
    {0x16, 0x24},
    {0x04, 0x10},
    {0x05, 0x00},
    {0x0B, 0x00},
    {0x0C, 0x00},
    {0x10, 0x1F},
    {0x11, 0x7F},
    {0x00, 0x80},
    {0x01, 0x3F},
    {0x06, 0x00},
    {0x13, 0x10},
    {0x1B, 0x0A},
    {0x1C, 0x6A},
    {0x44, 0x08},
};

}  // namespace

bool ES8311::initialize(TwoWire &i2c, uint8_t pa_pin, uint32_t sample_rate_hz) {
  i2c_ = &i2c;
  pa_pin_ = pa_pin;

  pinMode(pa_pin_, OUTPUT);
  digitalWrite(pa_pin_, LOW);

  i2c_->beginTransmission(codec_address);
  if (i2c_->endTransmission() != 0) {
    Serial.printf("ES8311 probe failed at I2C address 0x%02X\n", codec_address);
    return false;
  }

  for (const register_value &entry : open_sequence) {
    if (!write_reg(entry.reg, entry.value)) {
      Serial.printf("ES8311 initialization failed at register 0x%02X\n", entry.reg);
      return false;
    }
  }

  if (!configure_sample_rate(sample_rate_hz)) {
    return false;
  }

  if (!configure_input_gain()) {
    return false;
  }

  if (!start_codec()) {
    return false;
  }

  if (!set_output_volume(0xBF)) {
    return false;
  }

  uint8_t mute_reg = 0;
  if (!read_reg(0x31, mute_reg) || !write_reg(0x31, mute_reg & 0x9F)) {
    Serial.println("ES8311 DAC unmute failed");
    return false;
  }

  digitalWrite(pa_pin_, HIGH);
  return true;
}

bool ES8311::set_output_volume(uint8_t volume) {
  if (!write_reg(0x32, volume)) {
    Serial.println("ES8311 output volume configuration failed");
    return false;
  }
  return true;
}

bool ES8311::write_reg(uint8_t reg, uint8_t value) {
  i2c_->beginTransmission(codec_address);
  i2c_->write(reg);
  i2c_->write(value);
  return i2c_->endTransmission() == 0;
}

bool ES8311::read_reg(uint8_t reg, uint8_t &value) {
  i2c_->beginTransmission(codec_address);
  i2c_->write(reg);
  if (i2c_->endTransmission(false) != 0) {
    return false;
  }

  if (i2c_->requestFrom(codec_address, static_cast<size_t>(1)) != 1) {
    return false;
  }

  value = static_cast<uint8_t>(i2c_->read());
  return true;
}

bool ES8311::configure_sample_rate(uint32_t sample_rate_hz) {
  if (sample_rate_hz != 48000) {
    Serial.println("ES8311 example supports 48000 Hz only");
    return false;
  }

  // 12.288 MHz MCLK (256 * 48 kHz), 16-bit I2S, normal I2S format.
  constexpr register_value sample_rate_sequence[] = {
      {0x02, 0x00},
      {0x03, 0x10},
      {0x04, 0x10},
      {0x05, 0x00},
      {0x06, 0x03},
      {0x07, 0x00},
      {0x08, 0xFF},
      {0x09, 0x0C},
      {0x0A, 0x0C},
  };

  for (const register_value &entry : sample_rate_sequence) {
    if (!write_reg(entry.reg, entry.value)) {
      Serial.printf("ES8311 sample rate configuration failed at register 0x%02X\n", entry.reg);
      return false;
    }
  }

  return true;
}

bool ES8311::configure_input_gain() {
  // The open sequence's 0x24 is the codec startup value, not 24 dB gain.
  // Espressif's ES8311_MIC_GAIN_24DB enum maps to register value 0x04.
  if (!write_reg(0x16, input_gain_24db_code)) {
    Serial.println("Failed to configure ES8311 microphone gain");
    return false;
  }
  return true;
}

bool ES8311::start_codec() {
  constexpr register_value start_sequence[] = {
      {0x17, 0xBF},
      {0x0E, 0x02},
      {0x12, 0x00},
      {0x14, 0x1A},
      {0x0D, 0x01},
      {0x15, 0x40},
      {0x37, 0x08},
      {0x45, 0x00},
  };

  for (const register_value &entry : start_sequence) {
    if (!write_reg(entry.reg, entry.value)) {
      Serial.printf("ES8311 start failed at register 0x%02X\n", entry.reg);
      return false;
    }
  }

  return true;
}
