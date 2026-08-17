#include <Arduino.h>
#include <ESP_Video.h>
#include <Wire.h>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_rom_sys.h>
#include <driver/i2c_master.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_ops.h>
#include "lcd_panel.h"

static int lcd_width = 0;
static int lcd_height = 0;
static constexpr size_t lcd_bytes_per_pixel = sizeof(uint16_t);
static constexpr size_t display_buffer_count = 3;
static constexpr size_t capture_buffer_count = 2;
static constexpr uint8_t lcd_backlight_mode_register = 0x95;
static constexpr uint32_t lcd_backlight_setup_delay_ms = 100;
static constexpr uint32_t lcd_backlight_startup_delay_ms = 1000;

static ESPVideoClass video;
static ESPVideoCaptureDevClass capture_device;
static esp_lcd_panel_handle_t lcd_panel = nullptr;
static void *display_buffers[display_buffer_count] = {};
static size_t display_buffer_index = 0;
static SemaphoreHandle_t display_refresh_semaphore = nullptr;
static i2c_master_bus_handle_t board_i2c_bus = nullptr;
static i2c_master_dev_handle_t lcd_backlight_device = nullptr;
static bool camera_started = false;

static bool check_esp_err(esp_err_t err, const char *message) {
  if (err == ESP_OK) {
    return true;
  }

  Serial.printf("%s: %s\n", message, esp_err_to_name(err));
  return false;
}

static bool IRAM_ATTR lcd_frame_complete_callback(
  esp_lcd_panel_handle_t,
  esp_lcd_dpi_panel_event_data_t *,
  void *
)
{
  BaseType_t high_task_woken = pdFALSE;
  xSemaphoreGiveFromISR(display_refresh_semaphore, &high_task_woken);
  return high_task_woken == pdTRUE;
}

static bool start_board_i2c()
{
  Serial.println("LCD: board I2C initialization begin");
  if (!Wire1.begin(SDA, SCL, BOARD_I2C_FREQUENCY_HZ)) {
    Serial.println("Board I2C initialization failed: Wire1.begin()");
    return false;
  }

  const bool board_i2c_initialized = check_esp_err(
    i2c_master_get_bus_handle(
      static_cast<i2c_port_num_t>(BOARD_I2C_PORT),
      &board_i2c_bus
    ),
    "Board I2C bus handle acquisition failed"
  );
  Serial.printf("LCD: board I2C initialization %s\n", board_i2c_initialized ? "done" : "failed");
  return board_i2c_initialized;
}

static bool write_lcd_backlight_register(uint8_t reg, uint8_t value)
{
  const uint8_t command[] = {reg, value};
  Serial.printf("LCD: backlight register write begin, reg=0x%02X, value=0x%02X\n", reg, value);
  const bool write_succeeded = check_esp_err(
    i2c_master_transmit(lcd_backlight_device, command, sizeof(command), 100),
    "LCD backlight register write failed"
  );
  Serial.printf("LCD: backlight register write %s\n", write_succeeded ? "done" : "failed");
  return write_succeeded;
}

static bool set_lcd_backlight(uint8_t brightness)
{
  Serial.printf("LCD: backlight update begin, brightness=%u\n", brightness);
  if (lcd_backlight_device == nullptr) {
    Serial.printf("LCD: backlight device add begin, address=0x%02X\n", LCD_BACKLIGHT_I2C_ADDRESS);
    const i2c_device_config_t device_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = LCD_BACKLIGHT_I2C_ADDRESS,
      .scl_speed_hz = BOARD_I2C_FREQUENCY_HZ,
      .scl_wait_us = 0,
      .flags = {
        .disable_ack_check = false,
      },
    };
    const bool backlight_device_added = check_esp_err(
          i2c_master_bus_add_device(board_i2c_bus, &device_config, &lcd_backlight_device),
          "LCD backlight device initialization failed"
        );
    if (!backlight_device_added) {
      return false;
    }
    Serial.println("LCD: backlight device add done");
  }

  Serial.println("LCD: backlight transmit begin");
  const bool backlight_transmitted = write_lcd_backlight_register(
    LCD_BACKLIGHT_I2C_REGISTER,
    brightness
  );
  Serial.printf("LCD: backlight transmit %s\n", backlight_transmitted ? "done" : "failed");
  return backlight_transmitted;
}

static bool initialize_lcd_backlight()
{
  Serial.println("LCD: backlight chip initialization begin");
  if (!write_lcd_backlight_register(lcd_backlight_mode_register, 0x11)) {
    return false;
  }
  if (!write_lcd_backlight_register(lcd_backlight_mode_register, 0x17)) {
    return false;
  }
  if (!write_lcd_backlight_register(LCD_BACKLIGHT_I2C_REGISTER, 0x00)) {
    return false;
  }

  delay(lcd_backlight_setup_delay_ms);

  if (!write_lcd_backlight_register(LCD_BACKLIGHT_I2C_REGISTER, 0xFF)) {
    return false;
  }

  delay(lcd_backlight_startup_delay_ms);
  Serial.println("LCD: backlight chip initialization done");
  return true;
}

static bool start_lcd() {
  Serial.println("Initializing project LCD driver");
  Serial.println("LCD: backlight off begin");
  if (!set_lcd_backlight(0)) {
    return false;
  }
  Serial.println("LCD: backlight off done");
  if (!initialize_lcd_backlight()) {
    return false;
  }

  Serial.println("LCD: panel initialization begin");
  lcd_panel_info_t panel_info = {};
  if (!check_esp_err(
        lcd_panel_init(&lcd_panel, &panel_info),
        "Project LCD initialization failed"
      )) {
    return false;
  }
  lcd_width = static_cast<int>(panel_info.width);
  lcd_height = static_cast<int>(panel_info.height);
  Serial.printf("LCD: panel initialization done, resolution=%d x %d\n", lcd_width, lcd_height);

  Serial.println("LCD: frame buffer query begin");
  if (!check_esp_err(
        esp_lcd_dpi_panel_get_frame_buffer(
          lcd_panel,
          display_buffer_count,
          &display_buffers[0],
          &display_buffers[1],
          &display_buffers[2]
        ),
        "LCD frame buffer query failed"
      )) {
    return false;
  }
  Serial.println("LCD: frame buffer query done");

  display_refresh_semaphore = xSemaphoreCreateBinary();
  if (display_refresh_semaphore == nullptr) {
    Serial.println("LCD refresh semaphore creation failed");
    return false;
  }

  esp_lcd_dpi_panel_event_callbacks_t display_callbacks = {};
  display_callbacks.on_frame_buf_complete = lcd_frame_complete_callback;
  Serial.println("LCD: refresh callback registration begin");
  if (!check_esp_err(
        esp_lcd_dpi_panel_register_event_callbacks(lcd_panel, &display_callbacks, nullptr),
        "LCD refresh callback registration failed"
      )) {
    return false;
  }
  Serial.println("LCD: refresh callback registration done");

  display_buffer_index = 0;
  Serial.println("LCD: backlight on begin");
  if (!set_lcd_backlight(255)) {
    return false;
  }
  Serial.println("LCD: backlight on done");
  Serial.println("LCD: start complete");
  return true;
}

static bool start_camera() {
  Serial.println("Initializing OV5647 SCCB");
  ESPVideoCamConfigClass camera_config;
  if (!camera_config.begin(board_i2c_bus, BOARD_I2C_FREQUENCY_HZ)) {
    Serial.println("Camera SCCB initialization failed");
    return false;
  }

  ESPVideoCSIConfigClass csi_config;
  Serial.println("Configuring MIPI-CSI");
  if (!csi_config.begin(camera_config)) {
    Serial.println("MIPI-CSI configuration failed");
    return false;
  }
  Serial.println("Initializing OV5647");
  if (!video.begin(csi_config)) {
    Serial.println("OV5647 initialization failed");
    return false;
  }
  Serial.println("Opening camera capture device");
  if (!capture_device.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, capture_buffer_count)) {
    Serial.println("Camera capture device open failed");
    return false;
  }
  if (!capture_device.setFormat(ESP_VIDEO_FORMAT_RGB565)) {
    Serial.println("Camera RGB565 format configuration failed");
    return false;
  }
  if (!capture_device.startCapture()) {
    Serial.println("Camera capture start failed");
    return false;
  }

  return true;
}

static bool copy_frame_to_display_buffer(const ESPVideoBufferClass &buffer, void *display_buffer)
{
  const int frame_width = static_cast<int>(buffer.getWidth());
  const int frame_height = static_cast<int>(buffer.getHeight());
  if ((frame_width <= 0) || (frame_height <= 0)) {
    Serial.printf("Unsupported frame size: %d x %d\n", frame_width, frame_height);
    return false;
  }

  const int draw_width = min(frame_width, lcd_width);
  const int draw_height = min(frame_height, lcd_height);
  const int source_x = (frame_width - draw_width) / 2;
  const int source_y = (frame_height - draw_height) / 2;
  const int destination_x = (lcd_width - draw_width) / 2;
  const int destination_y = (lcd_height - draw_height) / 2;
  const size_t source_row_size = static_cast<size_t>(frame_width) * lcd_bytes_per_pixel;
  const size_t source_offset = static_cast<size_t>(source_y) * source_row_size
    + static_cast<size_t>(source_x) * lcd_bytes_per_pixel;
  const size_t required_buffer_size = source_offset
    + static_cast<size_t>(draw_height - 1) * source_row_size
    + static_cast<size_t>(draw_width) * lcd_bytes_per_pixel;
  if (buffer.size() < required_buffer_size) {
    Serial.printf(
      "Camera frame is truncated: %lu < %lu bytes\n",
      static_cast<unsigned long>(buffer.size()),
      static_cast<unsigned long>(required_buffer_size)
    );
    return false;
  }

  uint8_t *destination_buffer = static_cast<uint8_t *>(display_buffer);
  const size_t destination_row_size = static_cast<size_t>(lcd_width) * lcd_bytes_per_pixel;
  if (destination_y > 0) {
    memset(destination_buffer, 0, static_cast<size_t>(destination_y) * destination_row_size);
  }
  if (destination_y + draw_height < lcd_height) {
    memset(
      destination_buffer + static_cast<size_t>(destination_y + draw_height) * destination_row_size,
      0,
      static_cast<size_t>(lcd_height - destination_y - draw_height) * destination_row_size
    );
  }

  const uint8_t *source_buffer = buffer.data() + source_offset;
  for (int row = 0; row < draw_height; ++row) {
    uint8_t *destination_row = destination_buffer + static_cast<size_t>(destination_y + row) * destination_row_size;
    if (destination_x > 0) {
      memset(destination_row, 0, static_cast<size_t>(destination_x) * lcd_bytes_per_pixel);
    }
    destination_row += static_cast<size_t>(destination_x) * lcd_bytes_per_pixel;
    const size_t draw_row_size = static_cast<size_t>(draw_width) * lcd_bytes_per_pixel;
    memcpy(destination_row, source_buffer + static_cast<size_t>(row) * source_row_size, draw_row_size);
    if (destination_x + draw_width < lcd_width) {
      memset(
        destination_row + draw_row_size,
        0,
        static_cast<size_t>(lcd_width - destination_x - draw_width) * lcd_bytes_per_pixel
      );
    }
  }

  return true;
}

static bool display_camera_frame(const ESPVideoBufferClass &buffer)
{
  const size_t next_buffer_index = (display_buffer_index + 1) % display_buffer_count;
  if (!copy_frame_to_display_buffer(buffer, display_buffers[next_buffer_index])) {
    return false;
  }

  if (!check_esp_err(
        esp_lcd_panel_draw_bitmap(
          lcd_panel,
          0,
          0,
          lcd_width,
          lcd_height,
          display_buffers[next_buffer_index]
        ),
        "LCD frame switch failed"
      )) {
    return false;
  }

  display_buffer_index = next_buffer_index;
  return true;
}

static bool wait_for_next_display_refresh()
{
  return xSemaphoreTake(display_refresh_semaphore, portMAX_DELAY) == pdTRUE;
}

void setup() {
  esp_rom_printf("Starting MIPI-CSI display test\n");
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial initialized");

  if (!start_board_i2c()) {
    return;
  }
  if (!start_lcd()) {
    return;
  }
  camera_started = start_camera();
  if (camera_started) {
    Serial.println("OV5647 direct display started");
  }
}

void loop() {
  if (!camera_started) {
    delay(1000);
    return;
  }

  if (!wait_for_next_display_refresh()) {
    Serial.println("LCD refresh synchronization failed");
    delay(10);
    return;
  }

  ESPVideoBufferClass buffer = capture_device.captureBuffer();
  if (!buffer.valid()) {
    Serial.println("Camera capture failed");
    delay(10);
    return;
  }

  if (!display_camera_frame(buffer)) {
    delay(10);
  }
}
