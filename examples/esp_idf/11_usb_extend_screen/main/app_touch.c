/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "bsp/touch.h"
#include "esp_lcd_touch.h"

#include "app_lcd.h"
#include "app_usb.h"
#include "usb_descriptors.h"

static const char *TAG = "app_touch";
static esp_lcd_touch_handle_t tp;

static bool map_touch_point(uint16_t native_x, uint16_t native_y, uint16_t *usb_x, uint16_t *usb_y)
{
    if (native_x < EXAMPLE_LCD_OFFSET_Y ||
        native_x >= EXAMPLE_LCD_OFFSET_Y + EXAMPLE_LCD_CONTENT_V_RES ||
        native_y < EXAMPLE_LCD_OFFSET_X ||
        native_y >= EXAMPLE_LCD_OFFSET_X + EXAMPLE_LCD_CONTENT_H_RES) {
        return false;
    }

    *usb_x = native_x - EXAMPLE_LCD_OFFSET_Y;
    *usb_y = EXAMPLE_USB_V_RES - 1 - (native_y - EXAMPLE_LCD_OFFSET_X);
    return true;
}

static void app_touch_task(void *arg)
{
    bool send_press = false;

    while (1) {
        esp_lcd_touch_point_data_t touch_data[CONFIG_ESP_LCD_TOUCH_MAX_POINTS] = {0};
        uint8_t touchpad_cnt = 0;
        const bool touchpad_pressed = esp_lcd_touch_read_data(tp) == ESP_OK &&
                                      esp_lcd_touch_get_data(tp, touch_data, &touchpad_cnt,
                                                             CONFIG_ESP_LCD_TOUCH_MAX_POINTS) == ESP_OK &&
                                      touchpad_cnt > 0;
        hid_report_t report = {0};
        uint8_t report_count = 0;

        if (touchpad_pressed && touchpad_cnt > 0) {
            report.report_id = REPORT_ID_TOUCH;
            for (uint8_t i = 0; i < touchpad_cnt; i++) {
                uint16_t usb_x;
                uint16_t usb_y;
                if (!map_touch_point(touch_data[i].x, touch_data[i].y, &usb_x, &usb_y)) {
                    continue;
                }

                report.touch_report.data[report_count].index = touch_data[i].track_id;
                report.touch_report.data[report_count].press_down = 1;
                report.touch_report.data[report_count].x = usb_x;
                report.touch_report.data[report_count].y = usb_y;
                report.touch_report.data[report_count].width = touch_data[i].strength;
                report.touch_report.data[report_count].height = touch_data[i].strength;
                report_count++;

#if CONFIG_LOG_DEFAULT_LEVEL >= 4
                printf("(%d: %d, %d. %d) ", touch_data[i].track_id, usb_x, usb_y,
                       touch_data[i].strength);
#endif
            }

#if CONFIG_LOG_DEFAULT_LEVEL >= 4
            if (report_count > 0) {
                printf("\n");
            }
#endif
        }

        if (report_count > 0) {
            report.touch_report.cnt = report_count;
#if CFG_TUD_HID
            tinyusb_hid_keyboard_report(report);
#endif
            send_press = true;
        } else if (send_press) {
            report.report_id = REPORT_ID_TOUCH;
#if CFG_TUD_HID
            tinyusb_hid_keyboard_report(report);
#endif
            send_press = false;
            ESP_LOGD(TAG, "send release");
        }

        // Reading from the GT911 at a time shorter than this may result in false reports.
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

esp_err_t app_touch_init(void)
{
    const bsp_touch_config_t touch_config = {
        .flags = {
            .swap_xy = true,
        },
    };
    ESP_RETURN_ON_ERROR(bsp_touch_new(&touch_config, &tp), TAG, "Create touch device failed");
    xTaskCreate(app_touch_task, "app_touch_task", 4096, NULL, CONFIG_TOUCH_TASK_PRIORITY, NULL);
    return ESP_OK;
}
