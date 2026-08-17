/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "driver/jpeg_decode.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "app_lcd.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "app_lcd";

static esp_lcd_panel_handle_t display_handle;
static esp_lcd_panel_io_handle_t display_io;
static jpeg_decoder_handle_t jpgd_handle;
static uint8_t *jpeg_output_buffer;
static size_t jpeg_output_buffer_size;
static void *lcd_buffer[EXAMPLE_LCD_BUF_NUM];
static uint8_t buf_index;

static const jpeg_decode_cfg_t decode_cfg = {
    .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
    .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
};

// The Nano BSP exposes the MIPI panel in its portrait-native orientation. Rotate the
// landscape USB frame into that framebuffer so a landscape-mounted panel shows the
// expected 1024x576 content, with the remaining area left black.
static void rotate_frame_to_lcd(const uint16_t *source, uint16_t *destination)
{
    for (uint32_t source_y = 0; source_y < EXAMPLE_USB_V_RES; source_y++) {
        for (uint32_t source_x = 0; source_x < EXAMPLE_USB_H_RES; source_x++) {
            const uint32_t destination_x = EXAMPLE_LCD_OFFSET_X + EXAMPLE_USB_V_RES - 1 - source_y;
            const uint32_t destination_y = EXAMPLE_LCD_OFFSET_Y + source_x;
            destination[destination_y * EXAMPLE_LCD_H_RES + destination_x] =
                source[source_y * EXAMPLE_USB_H_RES + source_x];
        }
    }
}

void app_lcd_draw(uint8_t *buf, uint32_t len, uint16_t width, uint16_t height)
{
    static int fps_count;
    static int64_t start_time;
    uint32_t output_size = 0;

    if (width != EXAMPLE_USB_H_RES || height != EXAMPLE_USB_V_RES) {
        ESP_LOGW(TAG, "Ignore frame with unexpected size %ux%u, expected %ux%u",
                 width, height, EXAMPLE_USB_H_RES, EXAMPLE_USB_V_RES);
        return;
    }

    const esp_err_t ret = jpeg_decoder_process(jpgd_handle, &decode_cfg, buf, len,
                                               jpeg_output_buffer, jpeg_output_buffer_size,
                                               &output_size);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "JPEG decode failed: %s", esp_err_to_name(ret));
        return;
    }
    if (output_size < EXAMPLE_JPEG_BUF_LEN) {
        ESP_LOGW(TAG, "JPEG output is too small: %u bytes", output_size);
        return;
    }

    rotate_frame_to_lcd((const uint16_t *)jpeg_output_buffer, (uint16_t *)lcd_buffer[buf_index]);
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(display_handle, 0, 0, EXAMPLE_LCD_H_RES,
                                              EXAMPLE_LCD_V_RES, lcd_buffer[buf_index]));

    buf_index = (buf_index + 1) == EXAMPLE_LCD_BUF_NUM ? 0 : (buf_index + 1);

    fps_count++;
    if (fps_count == 50) {
        const int64_t end_time = esp_timer_get_time();
        const double fps = 1000000.0 / ((end_time - start_time) / 50.0);
        ESP_LOGI(TAG, "fps: %.2f", fps);
        start_time = end_time;
        fps_count = 0;
    }
}

esp_err_t app_lcd_init(void)
{
    const jpeg_decode_engine_cfg_t decode_eng_cfg = {
        .intr_priority = 1,
        .timeout_ms = 50,
    };
    ESP_RETURN_ON_ERROR(jpeg_new_decoder_engine(&decode_eng_cfg, &jpgd_handle), TAG, "Create JPEG decoder failed");

    jpeg_decode_memory_alloc_cfg_t output_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };
    jpeg_output_buffer = jpeg_alloc_decoder_mem(EXAMPLE_JPEG_BUF_LEN, &output_mem_cfg, &jpeg_output_buffer_size);
    ESP_RETURN_ON_FALSE(jpeg_output_buffer != NULL, ESP_ERR_NO_MEM, TAG, "Allocate JPEG output buffer failed");

    const bsp_display_config_t disp_config = {
        .num_fbs = EXAMPLE_LCD_BUF_NUM,
    };
    ESP_RETURN_ON_ERROR(bsp_display_new(&disp_config, &display_handle, &display_io), TAG, "Create Nano display failed");
    ESP_RETURN_ON_ERROR(bsp_display_backlight_on(), TAG, "Turn on LCD backlight failed");

#if EXAMPLE_LCD_BUF_NUM == 1
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(display_handle, 1, &lcd_buffer[0]), TAG, "Get LCD framebuffer failed");
#elif EXAMPLE_LCD_BUF_NUM == 2
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(display_handle, 2, &lcd_buffer[0], &lcd_buffer[1]), TAG, "Get LCD framebuffers failed");
#else
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(display_handle, 3, &lcd_buffer[0],&lcd_buffer[1], &lcd_buffer[2]), TAG, "Get LCD framebuffers failed");
#endif

    for (size_t i = 0; i < EXAMPLE_LCD_BUF_NUM; i++) {
        memset(lcd_buffer[i], 0, EXAMPLE_LCD_BUF_LEN);
    }

    ESP_LOGI(TAG, "USB input %ux%u rotated into %ux%u LCD content at (%u, %u) on native %ux%u",
             EXAMPLE_USB_H_RES, EXAMPLE_USB_V_RES, EXAMPLE_LCD_CONTENT_H_RES,
             EXAMPLE_LCD_CONTENT_V_RES, EXAMPLE_LCD_OFFSET_X, EXAMPLE_LCD_OFFSET_Y,
             EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    return ESP_OK;
}
