/**
 * MIPI-DSI LCD example for the Waveshare ESP32-P4-NANO-WIFI6-DB.
 *
 * Select the project-local panel controller in esp_panel_drivers_conf.h.
 */

#include <Arduino.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>

#include "esp_panel_drivers_conf.h"
#include <esp_display_panel.hpp>
#include <esp_lcd_panel_ops.h>

#if ESP_PANEL_DRIVERS_LCD_ENABLE_HX8394_LOCAL
#include "src/drivers/lcd/esp_panel_lcd_hx8394.hpp"
using selected_lcd_t = mipi_dsi::LCD_HX8394;
#elif ESP_PANEL_DRIVERS_LCD_ENABLE_ILI9881C_LOCAL
#include "src/drivers/lcd/esp_panel_lcd_ili9881c.hpp"
using selected_lcd_t = mipi_dsi::LCD_ILI9881C;
#elif ESP_PANEL_DRIVERS_LCD_ENABLE_JD9365_LOCAL
#include "src/drivers/lcd/port/esp_lcd_jd9365_local.h"
#include "src/drivers/lcd/esp_panel_lcd_jd9365_local.hpp"
using selected_lcd_t = mipi_dsi::LCD_JD9365_LOCAL;
#endif

using namespace esp_panel::drivers;

#if ESP_PANEL_DRIVERS_LCD_ENABLE_HX8394_LOCAL
static constexpr int lcd_width = 720;
static constexpr int lcd_height = 1280;
static constexpr int lcd_lane_rate_mbps = 700;
static constexpr int lcd_dpi_clock_mhz = 58;
static constexpr int lcd_hsync_pulse_width = 20;
static constexpr int lcd_hsync_back_porch = 20;
static constexpr int lcd_hsync_front_porch = 40;
static constexpr int lcd_vsync_pulse_width = 4;
static constexpr int lcd_vsync_back_porch = 10;
static constexpr int lcd_vsync_front_porch = 24;
#elif ESP_PANEL_DRIVERS_LCD_ENABLE_ILI9881C_LOCAL
static constexpr int lcd_width = 720;
static constexpr int lcd_height = 1280;
static constexpr int lcd_lane_rate_mbps = 1000;
static constexpr int lcd_dpi_clock_mhz = 80;
static constexpr int lcd_hsync_pulse_width = 50;
static constexpr int lcd_hsync_back_porch = 239;
static constexpr int lcd_hsync_front_porch = 33;
static constexpr int lcd_vsync_pulse_width = 30;
static constexpr int lcd_vsync_back_porch = 20;
static constexpr int lcd_vsync_front_porch = 2;
#elif ESP_PANEL_DRIVERS_LCD_ENABLE_JD9365_LOCAL
static constexpr int lcd_width = ESP_LCD_JD9365_LOCAL_HOR_RES;
static constexpr int lcd_height = ESP_LCD_JD9365_LOCAL_VER_RES;
static constexpr int lcd_lane_rate_mbps = ESP_LCD_JD9365_LOCAL_LANE_BIT_RATE_MBPS;
static constexpr int lcd_dpi_clock_mhz = ESP_LCD_JD9365_LOCAL_DPI_CLOCK_FREQ_MHZ;
static constexpr int lcd_hsync_pulse_width = ESP_LCD_JD9365_LOCAL_HSYNC_PULSE_WIDTH;
static constexpr int lcd_hsync_back_porch = ESP_LCD_JD9365_LOCAL_HSYNC_BACK_PORCH;
static constexpr int lcd_hsync_front_porch = ESP_LCD_JD9365_LOCAL_HSYNC_FRONT_PORCH;
static constexpr int lcd_vsync_pulse_width = ESP_LCD_JD9365_LOCAL_VSYNC_PULSE_WIDTH;
static constexpr int lcd_vsync_back_porch = ESP_LCD_JD9365_LOCAL_VSYNC_BACK_PORCH;
static constexpr int lcd_vsync_front_porch = ESP_LCD_JD9365_LOCAL_VSYNC_FRONT_PORCH;
#endif

static constexpr uint8_t lcd_backlight_mode_register = 0x95;
static constexpr uint32_t lcd_backlight_setup_delay_ms = 100;
static constexpr uint32_t lcd_backlight_startup_delay_ms = 1000;

static LCD *lcd = nullptr;
static bool board_i2c_started = false;

static LCD *create_native_lcd()
{
    const LCD::Config lcd_config = {
        .device = LCD::DeviceFullConfig{
            .reset_gpio_num = -1,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
            .bits_per_pixel = 16,
            .flags = {
                .reset_active_high = 0,
            },
            .vendor_config = nullptr,
        },
        .vendor = LCD::VendorPartialConfig{
            .hor_res = lcd_width,
            .ver_res = lcd_height,
        },
    };

    BusDSI::Config bus_config = {
        .host = BusDSI::HostFullConfig{
            .bus_id = 0,
            .num_data_lanes = 2,
            .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT,
            .lane_bit_rate_mbps = static_cast<float>(lcd_lane_rate_mbps),
        },
        .refresh_panel = BusDSI::RefreshPanelPartialConfig{
            .dpi_clock_freq_mhz = lcd_dpi_clock_mhz,
            .bits_per_pixel = 16,
            .h_size = lcd_width,
            .v_size = lcd_height,
            .hsync_pulse_width = lcd_hsync_pulse_width,
            .hsync_back_porch = lcd_hsync_back_porch,
            .hsync_front_porch = lcd_hsync_front_porch,
            .vsync_pulse_width = lcd_vsync_pulse_width,
            .vsync_back_porch = lcd_vsync_back_porch,
            .vsync_front_porch = lcd_vsync_front_porch,
        },
        .phy_ldo = BusDSI::PHY_LDO_PartialConfig{
            .chan_id = 3,
        },
    };

    auto *bus = new BusDSI(bus_config);
    auto *panel = new selected_lcd_t(bus, lcd_config);
    return panel;
}

static bool check_result(bool result, const char *message)
{
    if (result) {
        return true;
    }

    Serial.println(message);
    return false;
}

static bool start_board_i2c()
{
    if (board_i2c_started) {
        return true;
    }

    Serial.println("LCD: board I2C initialization begin");
    i2c_config_t config = {};
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = static_cast<gpio_num_t>(SDA);
    config.scl_io_num = static_cast<gpio_num_t>(SCL);
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = BOARD_I2C_FREQUENCY_HZ;

    const i2c_port_t port = static_cast<i2c_port_t>(BOARD_I2C_PORT);
    esp_err_t error = i2c_param_config(port, &config);
    if (error == ESP_OK) {
        error = i2c_driver_install(port, config.mode, 0, 0, 0);
    }

    board_i2c_started = error == ESP_OK;
    if (!board_i2c_started) {
        Serial.printf("LCD: board I2C initialization failed: %s\n", esp_err_to_name(error));
    }
    Serial.printf("LCD: board I2C initialization %s\n", board_i2c_started ? "done" : "failed");
    return board_i2c_started;
}

static bool write_lcd_backlight_register(uint8_t reg, uint8_t value)
{
    Serial.printf("LCD: backlight register write begin, reg=0x%02X, value=0x%02X\n", reg, value);
    const uint8_t data[] = {reg, value};
    const esp_err_t error = i2c_master_write_to_device(
        static_cast<i2c_port_t>(BOARD_I2C_PORT),
        LCD_BACKLIGHT_I2C_ADDRESS,
        data,
        sizeof(data),
        pdMS_TO_TICKS(100)
    );
    const bool write_succeeded = error == ESP_OK;
    if (!write_succeeded) {
        Serial.printf("LCD: backlight register write failed: %s\n", esp_err_to_name(error));
    }
    Serial.printf("LCD: backlight register write %s\n", write_succeeded ? "done" : "failed");
    return write_succeeded;
}

static bool set_lcd_backlight(uint8_t brightness)
{
    Serial.printf("LCD: backlight update begin, brightness=%u\n", brightness);
    if (!start_board_i2c()) {
        return false;
    }

    const bool write_succeeded = write_lcd_backlight_register(LCD_BACKLIGHT_I2C_REGISTER, brightness);
    Serial.printf("LCD: backlight update %s\n", write_succeeded ? "done" : "failed");
    return write_succeeded;
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

static bool start_lcd()
{
    Serial.println("Initializing project LCD driver");
    if (!start_board_i2c()) {
        return false;
    }
    if (!set_lcd_backlight(0)) {
        return false;
    }
    if (!initialize_lcd_backlight()) {
        return false;
    }

    lcd = create_native_lcd();
    if (lcd == nullptr) {
        Serial.println("LCD: ESP32_Display_Panel LCD creation failed");
        return false;
    }

    Serial.println("LCD: panel begin");
    if (!check_result(lcd->begin(), "LCD: panel begin failed")) {
        return false;
    }

    Serial.printf(
        "LCD: panel begin done, resolution=%d x %d\n",
        lcd->getFrameWidth(),
        lcd->getFrameHeight()
    );

    if (lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) {
        if (!check_result(lcd->setDisplayOnOff(true), "LCD: display-on failed")) {
            return false;
        }
    }

    if (!set_lcd_backlight(255)) {
        return false;
    }
    Serial.println("LCD: start complete");
    return true;
}

static bool show_dsi_pattern(LCD::DSI_ColorBarPattern pattern)
{
    if (lcd == nullptr) {
        return false;
    }

    return check_result(
        lcd->DSI_ColorBarPatternTest(pattern),
        "LCD DSI pattern update failed"
    );
}

void setup()
{
    Serial.begin(115200);

    if (!start_lcd()) {
        Serial.println("LCD: startup failed");
        return;
    }

    Serial.println("LCD: show MIPI-DSI color bar patterns");

}

void loop()
{
    show_dsi_pattern(LCD::DSI_ColorBarPattern::BAR_HORIZONTAL);
    delay(1000);
    show_dsi_pattern(LCD::DSI_ColorBarPattern::BAR_VERTICAL);
    delay(1000);
    show_dsi_pattern(LCD::DSI_ColorBarPattern::BER_VERTICAL);
    delay(1000);
    show_dsi_pattern(LCD::DSI_ColorBarPattern::NONE);
    delay(1000);
}
