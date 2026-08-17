/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>

#include "esp_codec_dev.h"
#include "esp_log.h"
#include "usb_device_uac.h"

#include "bsp/esp-bsp.h"
#include "usb_descriptors.h"

static const char *TAG = "app_uac";
static esp_codec_dev_handle_t speaker_codec;
static esp_codec_dev_handle_t microphone_codec;

static esp_err_t uac_device_output_cb(uint8_t *buf, size_t len, void *arg)
{
    return esp_codec_dev_write(speaker_codec, buf, (int)len) == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

static esp_err_t uac_device_input_cb(uint8_t *buf, size_t len, size_t *bytes_read, void *arg)
{
    if (esp_codec_dev_read(microphone_codec, buf, (int)len) != ESP_CODEC_DEV_OK) {
        *bytes_read = 0;
        ESP_LOGE(TAG, "Codec read failed");
        return ESP_FAIL;
    }

    *bytes_read = len;
    return ESP_OK;
}

static void uac_device_set_mute_cb(uint32_t mute, void *arg)
{
    ESP_LOGD(TAG, "uac_device_set_mute_cb: %" PRIu32, mute);
    if (speaker_codec != NULL) {
        esp_codec_dev_set_out_mute(speaker_codec, mute != 0);
    }
}

static void uac_device_set_volume_cb(uint32_t volume, void *arg)
{
    ESP_LOGD(TAG, "uac_device_set_volume_cb: %" PRIu32, volume);
    if (speaker_codec != NULL) {
        esp_codec_dev_set_out_vol(speaker_codec, (int)volume);
    }
}

esp_err_t app_uac_init(void)
{
#if CONFIG_UAC_SPEAKER_CHANNEL_NUM > 0
    speaker_codec = bsp_audio_codec_speaker_init();
    if (speaker_codec == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_codec_dev_sample_info_t speaker_fs = {
        .sample_rate = CONFIG_UAC_SAMPLE_RATE,
        .channel = CONFIG_UAC_SPEAKER_CHANNEL_NUM,
        .bits_per_sample = 16,
        .channel_mask = 0,
        .mclk_multiple = 0,
    };
    if (esp_codec_dev_open(speaker_codec, &speaker_fs) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Open speaker codec failed");
        return ESP_FAIL;
    }
#endif

#if CONFIG_UAC_MIC_CHANNEL_NUM > 0
    microphone_codec = bsp_audio_codec_microphone_init();
    if (microphone_codec == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_codec_dev_sample_info_t microphone_fs = {
        .sample_rate = CONFIG_UAC_SAMPLE_RATE,
        .channel = CONFIG_UAC_MIC_CHANNEL_NUM,
        .bits_per_sample = 16,
        .channel_mask = 0,
        .mclk_multiple = 0,
    };
    if (esp_codec_dev_open(microphone_codec, &microphone_fs) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Open microphone codec failed");
        return ESP_FAIL;
    }
#endif

    uac_device_config_t config = {
        .skip_tinyusb_init = true,
        .output_cb = uac_device_output_cb,
        .input_cb = uac_device_input_cb,
        .set_mute_cb = uac_device_set_mute_cb,
        .set_volume_cb = uac_device_set_volume_cb,
        .cb_ctx = NULL,
#if CONFIG_UAC_SPEAKER_CHANNEL_NUM > 0
        .spk_itf_num = ITF_NUM_AUDIO_STREAMING_SPK,
#endif
#if CONFIG_UAC_MIC_CHANNEL_NUM > 0
        .mic_itf_num = ITF_NUM_AUDIO_STREAMING_MIC,
#endif
    };

    return uac_device_init(&config);
}
