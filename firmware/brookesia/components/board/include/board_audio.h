/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_codec_dev.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "audio_player.h"
#include "file_iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_DEFAULT_SAMPLE_RATE           (16000)
#define CODEC_VOICE_SAMPLE_RATE             (24000)
#define CODEC_DEFAULT_BIT_WIDTH             (16)
/* ES8311 microphone gain, in dB. */
#define CODEC_DEFAULT_ADC_VOLUME            (30.0)
#define CODEC_DEFAULT_CHANNEL               (2)
#define CODEC_VOICE_INPUT_CHANNELS          (2)
#define CODEC_DEFAULT_VOLUME                (80)
#define BOARD_MIC1_CHANNEL_INDEX            (0)
#define BOARD_MIC_MAX_NUM                    (1)

/** Microphone wiring of the selected board. */
typedef struct {
    uint8_t  mic_num;                               /*!< Number of microphones, 0 to BOARD_MIC_MAX_NUM */
    uint8_t  mic_channel_index[BOARD_MIC_MAX_NUM];  /*!< Frame sample index of each microphone, MIC1 first */
    char     mic_label[BOARD_MIC_MAX_NUM][3];       /*!< Label of each microphone, such as "FL" */
    uint16_t mic_mask;                              /*!< Physical microphone input mask of the microphones */
    int8_t   echo_ref_channel_index;                /*!< Frame sample index of the AEC reference, -1 if none */
    uint16_t echo_ref_mask;                         /*!< Physical microphone input mask of the AEC reference */
} board_mic_layout_t;

/**
 * @brief Get the microphone wiring described by the board configuration.
 *
 * Works before board_codec_init(); the layout comes from the static device configuration.
 *
 * @param[out] layout Microphone layout
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_INVALID_ARG: layout is NULL
 *    - ESP_ERR_NOT_SUPPORTED: audio_adc does not match the single-microphone configuration
 */
esp_err_t board_audio_get_mic_layout(board_mic_layout_t *layout);

/**************************************************************************************************
 * Board audio interface
 * ES8311 playback, capture and the audio file player.
 **************************************************************************************************/
/**
 * @brief Player set mute.
 *
 * @param enable: true or false
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_codec_mute_set(bool enable);

/**
 * @brief Player set volume.
 *
 * @param volume: volume set
 * @param volume_set: volume set response
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_codec_volume_set(int volume, int *volume_set);

/**
 * @brief Player get volume.
 *
 * @return
 *   - volume: volume get
 */
int board_codec_volume_get(void);

/**
 * @brief Stop I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_codec_dev_stop(void);

/**
 * @brief Stop only the playback codec session.
 *
 * The capture session remains open.
 */
esp_err_t board_codec_output_stop(void);

/**
 * @brief Resume I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_codec_dev_resume(void);

/**
 * @brief Configure and open the output codec only.
 *
 * Changes only playback. While capture is active, its sample rate and bit depth
 * must match the capture format because the codec and I2S clocks are shared.
 *
 * @param rate Sample rate
 * @param bits_cfg Bits per sample
 * @param ch Output channel mode
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);

/**
 * @brief Configure ES8311 full-duplex voice audio over I2S STD.
 *
 * Capture returns stereo frames; microphone samples use BOARD_MIC1_CHANNEL_INDEX.
 * The board has no configured hardware echo reference.
 *
 * @param rate Shared physical sample rate
 * @param bits_cfg Bits per sample
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_codec_set_voice_fs(uint32_t rate, uint32_t bits_cfg);

/**
 * @brief Read data from recoder.
 *
 * @param audio_buffer: The pointer of receiving data buffer
 * @param len: Max data buffer length
 * @param bytes_read: Byte number that actually be read, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Write data to player.
 *
 * @param audio_buffer: The pointer of sent data buffer
 * @param len: Max data buffer length
 * @param bytes_written: Byte number that actually be sent, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t board_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);

/**
 * @brief Get the number of I2S TX underruns since the audio bus was initialized
 *
 * An underrun is counted each time the TX DMA runs out of new data (send queue overflow). The I2S
 * channel comes from esp_board_manager with the driver's default DMA layout (6 x 240 frames).
 *
 * @return Underrun count; wraps around on overflow
 */
uint32_t board_i2s_get_tx_underrun_count(void);

/**
 * @brief I2S DMA buffer layout of the audio bus
 *
 * esp_board_manager creates the channel with I2S_CHANNEL_DEFAULT_CONFIG(), so these mirror the
 * driver defaults. Buffered playback latency is BOARD_I2S_DMA_DESC_NUM * BOARD_I2S_DMA_FRAME_NUM frames.
 */
#define BOARD_I2S_DMA_DESC_NUM      (6)
#define BOARD_I2S_DMA_FRAME_NUM     (240)

/**
 * @brief Initialize codec play and record handle.
 *
 * Brings up the esp_board_manager audio_dac (ES8311) and audio_adc (ES8311) devices, which own the
 * shared I2S bus, the I2C control bus and the PA GPIO, then opens playback at the default format.
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t board_codec_init(void);

/**
 * @brief Initialize audio player task.
 *
 * @param path file path
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t board_player_init(void);

/**
 * @brief Delete audio player task.
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t board_player_del(void);

/**
 * @brief Initialize a file iterator instance
 *
 * @param path The file path for the iterator.
 * @param ret_instance A pointer to the file iterator instance to be returned.
 * @return
 *     - ESP_OK: Successfully initialized the file iterator instance.
 *     - ESP_FAIL: Failed to initialize the file iterator instance due to invalid parameters or memory allocation failure.
 */
esp_err_t board_file_instance_init(const char *path, file_iterator_instance_t **ret_instance);

/**
 * @brief Play the audio file at the specified index in the file iterator
 *
 * @param instance The file iterator instance.
 * @param index The index of the file to play within the iterator.
 * @return
 *     - ESP_OK: Successfully started playing the audio file.
 *     - ESP_FAIL: Failed to play the audio file due to invalid parameters or file access issues.
 */
esp_err_t board_player_play_index(file_iterator_instance_t *instance, int index);

/**
 * @brief Play the audio file specified by the file path
 *
 * @param file_path The path to the audio file to be played.
 * @return
 *     - ESP_OK: Successfully started playing the audio file.
 *     - ESP_FAIL: Failed to play the audio file due to file access issues.
 */
esp_err_t board_player_play_file(const char *file_path);

/**
 * @brief Register a callback function for the audio player
 *
 * @param cb The callback function to be registered.
 * @param user_data User data to be passed to the callback function.
 */
void board_player_register_callback(audio_player_cb_t cb, void *user_data);

/**
 * @brief Check if the specified audio file is currently playing
 *
 * @param file_path The path to the audio file to check.
 * @return
 *     - true: The specified audio file is currently playing.
 *     - false: The specified audio file is not currently playing.
 */
bool board_player_is_playing_by_path(const char *file_path);

/**
 * @brief Check if the audio file at the specified index is currently playing
 *
 * @param instance The file iterator instance.
 * @param index The index of the file to check.
 * @return
 *     - true: The audio file at the specified index is currently playing.
 *     - false: The audio file at the specified index is not currently playing.
 */
bool board_player_is_playing_by_index(file_iterator_instance_t *instance, int index);

#ifdef __cplusplus
}
#endif
