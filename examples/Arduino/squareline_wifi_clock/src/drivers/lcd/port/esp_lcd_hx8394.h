#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_idf_version.h"

#ifndef ESP_LCD_HX8394_VER_MAJOR
#define ESP_LCD_HX8394_VER_MAJOR (1)
#define ESP_LCD_HX8394_VER_MINOR (0)
#define ESP_LCD_HX8394_VER_PATCH (0)
#endif

#ifndef WAVESHARE_LCD_DPI_CONFIG_COLOR_FORMAT
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#ifndef LCD_COLOR_PIXEL_FORMAT_RGB565
#define LCD_COLOR_PIXEL_FORMAT_RGB565 LCD_COLOR_FMT_RGB565
#endif
#ifndef LCD_COLOR_PIXEL_FORMAT_RGB666
#define LCD_COLOR_PIXEL_FORMAT_RGB666 LCD_COLOR_FMT_RGB888
#endif
#ifndef LCD_COLOR_PIXEL_FORMAT_RGB888
#define LCD_COLOR_PIXEL_FORMAT_RGB888 LCD_COLOR_FMT_RGB888
#endif
#define WAVESHARE_LCD_DPI_CONFIG_COLOR_FORMAT(px_format) .in_color_format = px_format
#define WAVESHARE_LCD_DPI_CONFIG_DMA2D
#else
#define WAVESHARE_LCD_DPI_CONFIG_COLOR_FORMAT(px_format) .pixel_format = px_format
#define WAVESHARE_LCD_DPI_CONFIG_DMA2D .flags.use_dma2d = true,
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create LCD panel for model HX8394
 *
 * @note  Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for initialization sequence code.
 *
 * @param[in]  io LCD panel IO handle
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *      - ESP_ERR_INVALID_ARG   if parameter is invalid
 *      - ESP_OK                on success
 *      - Otherwise             on fail
 */
esp_err_t esp_lcd_new_panel_hx8394(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief MIPI DSI bus configuration structure
 *
 */
#define HX8394_PANEL_BUS_DSI_2CH_CONFIG()                 \
    {                                                     \
        .bus_id = 0,                                      \
        .num_data_lanes = 2,                              \
        .phy_clk_src = 0,      \
        .lane_bit_rate_mbps = 700,                        \
    }

/**
 * @brief MIPI DBI panel IO configuration structure
 *
 */
#define HX8394_PANEL_IO_DBI_CONFIG() \
    {                                 \
        .virtual_channel = 0,         \
        .lcd_cmd_bits = 8,            \
        .lcd_param_bits = 8,          \
    }

/**
 * @brief MIPI DPI configuration structure
 *
 * @note  refresh_rate = (dpi_clock_freq_mhz * 1000000) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *                                                      / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 *
 */
#define HX8394_720_1280_PANEL_30HZ_DPI_CONFIG(px_format)            \
    {                                                            \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,             \
        .dpi_clock_freq_mhz = 58,                                \
        .virtual_channel = 0,                                    \
        WAVESHARE_LCD_DPI_CONFIG_COLOR_FORMAT(px_format),                               \
        .num_fbs = 1,                                            \
        .video_timing = {                                        \
            .h_size = 720,                                      \
            .v_size = 1280,                                      \
            .hsync_back_porch = 20,                             \
            .hsync_pulse_width = 20,                             \
            .hsync_front_porch = 40,                             \
            .vsync_back_porch = 10,                              \
            .vsync_pulse_width = 4,                              \
            .vsync_front_porch = 24,                             \
        },                                                       \
        WAVESHARE_LCD_DPI_CONFIG_DMA2D                                 \
    }
#endif

#ifdef __cplusplus
}
#endif

