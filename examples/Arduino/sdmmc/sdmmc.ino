#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

static const char *get_card_type_name(uint8_t card_type) {
  switch (card_type) {
    case CARD_MMC:
      return "MMC";
    case CARD_SD:
      return "SDSC";
    case CARD_SDHC:
      return "SDHC";
    default:
      return "Unknown";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting SDMMC test");

  Serial.println("Enabling SD card power");
  pinMode(BOARD_SDMMC_POWER_PIN, OUTPUT);
  digitalWrite(BOARD_SDMMC_POWER_PIN, BOARD_SDMMC_POWER_ON_LEVEL);
  delay(10);

  Serial.println("Configuring SDMMC pins for 4-bit mode");
  if (!SD_MMC.setPins(
        BOARD_SDMMC_CLK, BOARD_SDMMC_CMD,
        BOARD_SDMMC_D0, BOARD_SDMMC_D1, BOARD_SDMMC_D2, BOARD_SDMMC_D3
      )) {
    Serial.println("SDMMC pin configuration failed");
    return;
  }

  Serial.println("Mounting SD card");
  if (!SD_MMC.begin("/sdcard", false)) {
    Serial.println("SD card mount failed");
    return;
  }

  const uint8_t card_type = SD_MMC.cardType();
  if (card_type == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.printf("SD card detected: %s\n", get_card_type_name(card_type));
  Serial.printf("Card size: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
  Serial.printf("Filesystem total: %llu MB\n", SD_MMC.totalBytes() / (1024ULL * 1024ULL));
  Serial.printf("Filesystem used: %llu MB\n", SD_MMC.usedBytes() / (1024ULL * 1024ULL));

  Serial.println("Writing /arduino_test.txt");
  File file = SD_MMC.open("/arduino_test.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Test file open failed");
    return;
  }
  const size_t bytes_written = file.println("ESP32-P4 SDMMC test");
  file.close();
  Serial.printf("Test file write complete: %u bytes\n", bytes_written);

  Serial.println("Reading /arduino_test.txt");
  file = SD_MMC.open("/arduino_test.txt", FILE_READ);
  if (!file) {
    Serial.println("Test file read failed");
    return;
  }
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  Serial.println("SDMMC test completed successfully");
}

void loop() {
  static uint32_t last_status_time = 0;
  if (millis() - last_status_time >= 5000) {
    last_status_time = millis();
    Serial.println("SDMMC is mounted and idle");
  }
}
