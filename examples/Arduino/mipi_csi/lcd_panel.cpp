/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Project-owned ESP32-P4-NANO-WIFI6-DB LCD setup. The panel-specific controller
 * implementations are kept in the local driver files so this example does
 * not pull in ESP32_Display_Panel and its legacy I2C driver.
 */

#include <Arduino.h>

#include "lcd_panel.h"

#include "esp_lcd_hx8394.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_jd9365.h"
#include "esp_panel_lcd_vendor_types.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"

namespace {

constexpr int dsi_phy_ldo_channel = 3;
constexpr int dsi_phy_ldo_voltage_mv = 2500;
constexpr size_t display_buffer_count = 3;

static esp_ldo_channel_handle_t dsi_phy_ldo = nullptr;
static esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
static esp_lcd_panel_io_handle_t dbi_io = nullptr;
static esp_lcd_panel_handle_t lcd_panel = nullptr;

static void configure_dpi_panel(
  esp_lcd_dpi_panel_config_t *dpi_config,
  float dpi_clock_freq_mhz,
  const esp_lcd_video_timing_t &video_timing,
  bool use_dma2d
)
{
  dpi_config->virtual_channel = 0;
  dpi_config->dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
  dpi_config->dpi_clock_freq_mhz = dpi_clock_freq_mhz;
  dpi_config->pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
  dpi_config->num_fbs = display_buffer_count;
  dpi_config->video_timing = video_timing;
  dpi_config->flags.use_dma2d = use_dma2d;
}

static void configure_dsi_bus(
  esp_lcd_dsi_bus_config_t *dsi_bus_config,
  float lane_bit_rate_mbps
)
{
  dsi_bus_config->bus_id = 0;
  dsi_bus_config->num_data_lanes = 2;
  dsi_bus_config->phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT;
  dsi_bus_config->lane_bit_rate_mbps = lane_bit_rate_mbps;
}

static esp_err_t prepare_panel_config(
  lcd_panel_type_t panel_type,
  esp_lcd_dsi_bus_config_t *dsi_bus_config,
  esp_lcd_dpi_panel_config_t *dpi_config,
  lcd_panel_info_t *panel_info,
  const char **panel_name
)
{
  switch (panel_type) {
    case lcd_panel_type_jd9365:
      configure_dsi_bus(dsi_bus_config, 1500);
      configure_dpi_panel(
        dpi_config,
        80,
        {800, 1280, 20, 20, 40, 4, 12, 30},
        false
      );
      panel_info->width = 800;
      panel_info->height = 1280;
      *panel_name = "JD9365";
      return ESP_OK;

    case lcd_panel_type_hx8394:
      configure_dsi_bus(dsi_bus_config, 700);
      configure_dpi_panel(
        dpi_config,
        58,
        {720, 1280, 20, 20, 40, 4, 10, 24},
        true
      );
      panel_info->width = 720;
      panel_info->height = 1280;
      *panel_name = "HX8394";
      return ESP_OK;

    case lcd_panel_type_ili9881c:
      configure_dsi_bus(dsi_bus_config, 1000);
      configure_dpi_panel(
        dpi_config,
        80,
        {720, 1280, 50, 239, 33, 30, 20, 2},
        true
      );
      panel_info->width = 720;
      panel_info->height = 1280;
      *panel_name = "ILI9881C";
      return ESP_OK;

    default:
      return ESP_ERR_INVALID_ARG;
  }
}

static void cleanup_lcd()
{
  if (lcd_panel != nullptr) {
    esp_lcd_panel_del(lcd_panel);
    lcd_panel = nullptr;
  }
  if (dbi_io != nullptr) {
    esp_lcd_panel_io_del(dbi_io);
    dbi_io = nullptr;
  }
  if (dsi_bus != nullptr) {
    esp_lcd_del_dsi_bus(dsi_bus);
    dsi_bus = nullptr;
  }
  if (dsi_phy_ldo != nullptr) {
    esp_ldo_release_channel(dsi_phy_ldo);
    dsi_phy_ldo = nullptr;
  }
}

static void report_lcd_error(const char *stage, esp_err_t err)
{
  Serial.printf("LCD: %s failed: %s\n", stage, esp_err_to_name(err));
}

}  // namespace

extern "C" esp_err_t lcd_panel_get_info(lcd_panel_info_t *panel_info)
{
  if (panel_info == nullptr) {
    Serial.println("LCD: invalid panel info argument");
    return ESP_ERR_INVALID_ARG;
  }

  esp_lcd_dsi_bus_config_t dsi_bus_config = {};
  esp_lcd_dpi_panel_config_t dpi_config = {};
  const char *panel_name = nullptr;
  return prepare_panel_config(
    LCD_PANEL_TYPE,
    &dsi_bus_config,
    &dpi_config,
    panel_info,
    &panel_name
  );
}

extern "C" esp_err_t lcd_panel_create(esp_lcd_panel_handle_t *ret_panel, lcd_panel_info_t *panel_info)
{
  if ((ret_panel == nullptr) || (panel_info == nullptr)) {
    Serial.println("LCD: invalid panel output arguments");
    return ESP_ERR_INVALID_ARG;
  }
  *ret_panel = nullptr;
  *panel_info = {};

  esp_lcd_dsi_bus_config_t dsi_bus_config = {};
  esp_lcd_dpi_panel_config_t dpi_config = {};
  const char *panel_name = nullptr;
  esp_err_t err = prepare_panel_config(
    LCD_PANEL_TYPE,
    &dsi_bus_config,
    &dpi_config,
    panel_info,
    &panel_name
  );
  if (err != ESP_OK) {
    report_lcd_error("panel type configuration", err);
    return err;
  }

  const esp_ldo_channel_config_t ldo_config = {
    .chan_id = dsi_phy_ldo_channel,
    .voltage_mv = dsi_phy_ldo_voltage_mv,
  };
  Serial.println("LCD: DSI PHY LDO acquire begin");
  err = esp_ldo_acquire_channel(&ldo_config, &dsi_phy_ldo);
  if (err != ESP_OK) {
    report_lcd_error("DSI PHY LDO acquire", err);
    return err;
  }
  Serial.println("LCD: DSI PHY LDO acquire done");

  Serial.println("LCD: DSI bus creation begin");
  err = esp_lcd_new_dsi_bus(&dsi_bus_config, &dsi_bus);
  if (err != ESP_OK) {
    report_lcd_error("DSI bus creation", err);
    cleanup_lcd();
    return err;
  }
  Serial.println("LCD: DSI bus creation done");

  const esp_lcd_dbi_io_config_t dbi_config = {
    .virtual_channel = 0,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
  };
  Serial.println("LCD: DBI IO creation begin");
  err = esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &dbi_io);
  if (err != ESP_OK) {
    report_lcd_error("DBI IO creation", err);
    cleanup_lcd();
    return err;
  }
  Serial.println("LCD: DBI IO creation done");

  Serial.println("LCD: DPI configuration prepared");

  esp_panel_lcd_vendor_config_t jd9365_vendor_config = {};
  hx8394_vendor_config_t hx8394_vendor_config = {};
  ili9881c_vendor_config_t ili9881c_vendor_config = {};
  void *vendor_config = nullptr;
  switch (LCD_PANEL_TYPE) {
    case lcd_panel_type_jd9365:
      jd9365_vendor_config.mipi_config.lane_num = 2;
      jd9365_vendor_config.mipi_config.dsi_bus = dsi_bus;
      jd9365_vendor_config.mipi_config.dpi_config = &dpi_config;
      vendor_config = &jd9365_vendor_config;
      break;

    case lcd_panel_type_hx8394:
      hx8394_vendor_config.mipi_config.lane_num = 2;
      hx8394_vendor_config.mipi_config.dsi_bus = dsi_bus;
      hx8394_vendor_config.mipi_config.dpi_config = &dpi_config;
      vendor_config = &hx8394_vendor_config;
      break;

    case lcd_panel_type_ili9881c:
      ili9881c_vendor_config.mipi_config.lane_num = 2;
      ili9881c_vendor_config.mipi_config.dsi_bus = dsi_bus;
      ili9881c_vendor_config.mipi_config.dpi_config = &dpi_config;
      vendor_config = &ili9881c_vendor_config;
      break;

    default:
      cleanup_lcd();
      return ESP_ERR_INVALID_ARG;
  }

  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = -1,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
    .bits_per_pixel = 16,
    .flags = {
      .reset_active_high = 0,
    },
    .vendor_config = vendor_config,
  };
  Serial.printf("LCD: %s panel creation begin\n", panel_name);
  switch (LCD_PANEL_TYPE) {
    case lcd_panel_type_jd9365:
      err = esp_lcd_new_panel_jd9365(dbi_io, &panel_config, &lcd_panel);
      break;

    case lcd_panel_type_hx8394:
      err = esp_lcd_new_panel_hx8394(dbi_io, &panel_config, &lcd_panel);
      break;

    case lcd_panel_type_ili9881c:
      err = esp_lcd_new_panel_ili9881c(dbi_io, &panel_config, &lcd_panel);
      break;

    default:
      err = ESP_ERR_INVALID_ARG;
      break;
  }
  if (err != ESP_OK) {
    report_lcd_error("panel creation", err);
    cleanup_lcd();
    return err;
  }
  Serial.printf("LCD: %s panel creation done\n", panel_name);

  *ret_panel = lcd_panel;
  Serial.printf("LCD: %s panel creation complete\n", panel_name);
  return ESP_OK;
}

extern "C" esp_err_t lcd_panel_init(esp_lcd_panel_handle_t *ret_panel, lcd_panel_info_t *panel_info)
{
  if ((ret_panel == nullptr) || (panel_info == nullptr)) {
    Serial.println("LCD: invalid panel output arguments");
    return ESP_ERR_INVALID_ARG;
  }
  *ret_panel = nullptr;

  const char *panel_name = "LCD";
  switch (LCD_PANEL_TYPE) {
    case lcd_panel_type_jd9365:
      panel_name = "JD9365";
      break;
    case lcd_panel_type_hx8394:
      panel_name = "HX8394";
      break;
    case lcd_panel_type_ili9881c:
      panel_name = "ILI9881C";
      break;
    default:
      break;
  }

  esp_lcd_panel_handle_t created_panel = nullptr;
  esp_err_t err = lcd_panel_create(&created_panel, panel_info);
  if (err != ESP_OK) {
    return err;
  }

  Serial.println("LCD: panel reset begin");
  err = esp_lcd_panel_reset(created_panel);
  if (err != ESP_OK) {
    report_lcd_error("panel reset", err);
    cleanup_lcd();
    return err;
  }
  Serial.println("LCD: panel reset done");

  Serial.println("LCD: panel initialization begin");
  err = esp_lcd_panel_init(created_panel);
  if (err != ESP_OK) {
    report_lcd_error("panel initialization", err);
    cleanup_lcd();
    return err;
  }
  Serial.println("LCD: panel initialization done");

  Serial.println("LCD: display-on command begin");
  err = esp_lcd_panel_disp_on_off(created_panel, true);
  if (err != ESP_OK) {
    report_lcd_error("display-on command", err);
    cleanup_lcd();
    return err;
  }
  Serial.println("LCD: display-on command done");

  *ret_panel = created_panel;
  Serial.printf("LCD: %s driver initialization complete\n", panel_name);
  return ESP_OK;
}
