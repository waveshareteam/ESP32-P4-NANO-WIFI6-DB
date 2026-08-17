/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal Arduino ES8311 control helper for ESP32-P4-NANO-WIFI6-DB.
 * The register setup follows Espressif's esp_codec_dev ES8311 driver.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

class ES8311 {
public:
  bool initialize(TwoWire &i2c, uint8_t pa_pin, uint32_t sample_rate_hz);
  bool set_output_volume(uint8_t volume);

private:
  // ESP-IDF BSP uses the 8-bit write address 0x30; Wire uses 7-bit 0x18.
  static constexpr uint8_t codec_address = 0x18;

  bool write_reg(uint8_t reg, uint8_t value);
  bool read_reg(uint8_t reg, uint8_t &value);
  bool configure_sample_rate(uint32_t sample_rate_hz);
  bool configure_input_gain();
  bool start_codec();

  TwoWire *i2c_ = nullptr;
  uint8_t pa_pin_ = 0xFF;
};
