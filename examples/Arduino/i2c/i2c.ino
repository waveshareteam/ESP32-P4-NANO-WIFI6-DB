#include <Arduino.h>
#include <Wire.h>

void scan_i2c_bus() {
  uint8_t device_count = 0;

  Serial.println("Scanning I2C bus...");
  for (uint8_t address = 1; address < 0x7f; ++address) {
    Wire1.beginTransmission(address);
    const uint8_t error = Wire1.endTransmission();
    if (error == 0) {
      Serial.printf("Device found at 0x%02X\n", address);
      ++device_count;
    } else if (error != 2) {
      Serial.printf("Error %u at 0x%02X\n", error, address);
    }
  }
  Serial.printf("Scan complete: %u device(s) found\n", device_count);
}

void setup() {
  Serial.begin(115200);
  Wire1.begin(SDA, SCL, BOARD_I2C_FREQUENCY_HZ);
}

void loop() {
  scan_i2c_bus();
  delay(5000);
}
