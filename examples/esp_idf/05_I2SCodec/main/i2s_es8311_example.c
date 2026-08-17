/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"
#include "example_config.h"

#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#else
#include "es8311.h"
#endif

static const char *TAG = "i2s_es8311";
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };
#if !(CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4)
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
#endif

/* Import music file as buffer */
#if CONFIG_EXAMPLE_MODE_MUSIC
extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");
#endif

static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BSP_POWER_AMP_IO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(BSP_POWER_AMP_IO, 1);
}

static esp_err_t es8311_codec_init(void)
{
    ESP_RETURN_ON_ERROR(bsp_extra_codec_init(), TAG,
                        "init ESP32-P4-NANO-WIFI6-DB BSP codec failed");
    ESP_RETURN_ON_ERROR(bsp_extra_codec_set_fs(EXAMPLE_SAMPLE_RATE, 16, I2S_SLOT_MODE_STEREO), TAG,
                        "set ESP32-P4-NANO-WIFI6-DB BSP codec format failed");
    ESP_RETURN_ON_ERROR(bsp_extra_codec_volume_set(EXAMPLE_VOICE_VOLUME, NULL), TAG,
                        "set ESP32-P4-NANO-WIFI6-DB BSP codec volume failed");
#if CONFIG_EXAMPLE_MODE_ECHO
    ESP_RETURN_ON_ERROR(bsp_extra_codec_mic_gain_set(EXAMPLE_MIC_GAIN_DB), TAG,
                        "set ESP32-P4-NANO-WIFI6-DB microphone gain failed");
#endif
    return ESP_OK;
}

#if CONFIG_EXAMPLE_MODE_MUSIC
static void i2s_music(void *args)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;
    const size_t music_size = music_pcm_end - music_pcm_start;

    while (1) {
        data_ptr = (uint8_t *)music_pcm_start;
        ret = bsp_extra_i2s_write(data_ptr, music_size, &bytes_write, portMAX_DELAY);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_write > 0) {
            ESP_LOGI(TAG, "[music] i2s music played, %u bytes are written.", (unsigned int)bytes_write);
        } else {
            ESP_LOGE(TAG, "[music] i2s music play failed.");
            abort();
        }
        data_ptr = (uint8_t *)music_pcm_start;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

#else
static void i2s_echo(void *args)
{
    int *mic_data = malloc(EXAMPLE_RECV_BUF_SIZE);
    if (!mic_data) {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;
    ESP_LOGI(TAG, "[echo] Echo start");

    while (1) {
        memset(mic_data, 0, EXAMPLE_RECV_BUF_SIZE);
        ret = bsp_extra_i2s_read(mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_read, 1000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s read failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
#if EXAMPLE_HEADPHONE_MODE
        int16_t *samples = (int16_t *)mic_data;
        size_t sample_count = bytes_read / sizeof(*samples);
        for (size_t i = 0; i + 1 < sample_count; i += 2) {
            samples[i + 1] = samples[i];
        }
#endif
#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
        ret = bsp_extra_i2s_write(mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_write, 1000);
#else
        ret = i2s_channel_write(tx_handle, mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_write, 1000);
#endif
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_read != bytes_write) {
            ESP_LOGW(TAG, "[echo] %u bytes read but only %u bytes are written", (unsigned int)bytes_read, (unsigned int)bytes_write);
        }
    }
    vTaskDelete(NULL);
}
#endif

void app_main(void)
{
    gpio_init();
    if (es8311_codec_init() != ESP_OK) {
        ESP_LOGE(TAG, "es8311 codec init failed");
        abort();
    } else {
        ESP_LOGI(TAG, "es8311 codec init success");
    }
#if CONFIG_EXAMPLE_MODE_MUSIC
    xTaskCreate(i2s_music, "i2s_music", 4096, NULL, 5, NULL);
#else
    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);
#endif
}
