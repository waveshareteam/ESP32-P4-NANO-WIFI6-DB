/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESP_LCD_H
#define ESP_LCD_H

#include <stdint.h>

#include "esp_err.h"
#include "bsp/esp-bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE_USB_H_RES                   (CONFIG_USB_EXTEND_SCREEN_HEIGHT)
#define EXAMPLE_USB_V_RES                   (CONFIG_USB_EXTEND_SCREEN_WIDTH)

#define EXAMPLE_LCD_H_RES                   (BSP_LCD_H_RES)
#define EXAMPLE_LCD_V_RES                   (BSP_LCD_V_RES)
#define EXAMPLE_LCD_CONTENT_H_RES           (EXAMPLE_USB_V_RES)
#define EXAMPLE_LCD_CONTENT_V_RES           (EXAMPLE_USB_H_RES)
#define EXAMPLE_LCD_OFFSET_X                ((EXAMPLE_LCD_H_RES - EXAMPLE_LCD_CONTENT_H_RES) / 2)
#define EXAMPLE_LCD_OFFSET_Y                ((EXAMPLE_LCD_V_RES - EXAMPLE_LCD_CONTENT_V_RES) / 2)

#define EXAMPLE_LCD_BUF_NUM                 (CONFIG_BSP_LCD_DPI_BUFFER_NUMS)

#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB565
#define EXAMPLE_LCD_BIT_PER_PIXEL           (BSP_LCD_BITS_PER_PIXEL)
#elif CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
#error "The USB extend screen example requires RGB565 LCD output"
#endif

#define EXAMPLE_LCD_BUF_LEN                 (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * EXAMPLE_LCD_BIT_PER_PIXEL / 8)
#define EXAMPLE_JPEG_BUF_LEN                (EXAMPLE_USB_H_RES * EXAMPLE_USB_V_RES * 2)

#if EXAMPLE_LCD_CONTENT_H_RES > EXAMPLE_LCD_H_RES || EXAMPLE_LCD_CONTENT_V_RES > EXAMPLE_LCD_V_RES
#error "The USB input image does not fit in the selected Nano LCD panel"
#endif

/**
 * @brief Initialize the LCD panel.
 *
 * This function initializes the LCD panel with the provided panel handle. It powers on the LCD,
 * installs the LCD driver, configures the bus, and sets up the panel.
 *
 * @return
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t app_lcd_init(void);

void app_lcd_draw(uint8_t *buf, uint32_t len, uint16_t width, uint16_t height);

#ifdef __cplusplus
}
#endif

#endif
