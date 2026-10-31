#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:UartTool"
#include "esp_lib_utils.h"

#include <inttypes.h>
#include "nvs.h"
#include "uart_tool_config.hpp"

#define NVS_NAMESPACE "uart_tool"

namespace esp_brookesia::apps {

// RS485/RS232 are hidden in the UI, so a stored value for them falls back to the default.
static bool is_supported_mode(int32_t mode)
{
    return mode == static_cast<int32_t>(UartToolMode::Uart);
}

static bool is_supported_baud(int32_t baud)
{
    static const int32_t supported[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800};
    for (int32_t value : supported) {
        if (baud == value) {
            return true;
        }
    }
    return false;
}

static void load_i32(nvs_handle_t handle, const char *key, int32_t &value)
{
    int32_t tmp = 0;
    if (nvs_get_i32(handle, key, &tmp) == ESP_OK) {
        value = tmp;
    }
}

UartToolConfig uart_tool_config_load()
{
    UartToolConfig config;   // defaults

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_UTILS_LOGW("NVS open failed (%s), use defaults", esp_err_to_name(err));
        return config;
    }

    int32_t mode = static_cast<int32_t>(config.mode), baud = config.baud;
    int32_t data_bits = config.data_bits, stop_bits = config.stop_bits, parity = config.parity;
    int32_t hex_rx = config.hex_rx, hex_tx = config.hex_tx;
    load_i32(handle, "mode", mode);
    load_i32(handle, "baud", baud);
    load_i32(handle, "data_bits", data_bits);
    load_i32(handle, "stop_bits", stop_bits);
    load_i32(handle, "parity", parity);
    load_i32(handle, "hex_rx", hex_rx);
    load_i32(handle, "hex_tx", hex_tx);
    nvs_close(handle);

    bool corrected = false;
    auto use_default = [&](int32_t &value, int32_t default_value, const char *name) {
        ESP_UTILS_LOGW("Invalid NVS %s=%" PRId32 ", use default %" PRId32, name, value, default_value);
        value = default_value;
        corrected = true;
    };

    const UartToolConfig defaults;
    if (!is_supported_mode(mode)) {
        use_default(mode, static_cast<int32_t>(defaults.mode), "mode");
    }
    if (!is_supported_baud(baud)) {
        use_default(baud, defaults.baud, "baud");
    }
    if (data_bits < UART_DATA_5_BITS || data_bits > UART_DATA_8_BITS) {
        use_default(data_bits, defaults.data_bits, "data_bits");
    }
    if (stop_bits != UART_STOP_BITS_1 &&
        stop_bits != UART_STOP_BITS_1_5 &&
        stop_bits != UART_STOP_BITS_2) {
        use_default(stop_bits, defaults.stop_bits, "stop_bits");
    }
    if (parity != UART_PARITY_DISABLE &&
        parity != UART_PARITY_EVEN &&
        parity != UART_PARITY_ODD) {
        use_default(parity, defaults.parity, "parity");
    }

    config.mode = static_cast<UartToolMode>(mode);
    config.baud = baud;
    config.data_bits = static_cast<uart_word_length_t>(data_bits);
    config.stop_bits = static_cast<uart_stop_bits_t>(stop_bits);
    config.parity = static_cast<uart_parity_t>(parity);
    config.hex_rx = (hex_rx != 0);
    config.hex_tx = (hex_tx != 0);
    if (corrected) {
        uart_tool_config_save(config);
    }
    return config;
}

bool uart_tool_config_save(const UartToolConfig &config)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    ESP_UTILS_CHECK_FALSE_RETURN(err == ESP_OK, false, "NVS open failed: %s", esp_err_to_name(err));

    bool ok = true;
    ok &= (nvs_set_i32(handle, "mode", static_cast<int32_t>(config.mode)) == ESP_OK);
    ok &= (nvs_set_i32(handle, "baud", config.baud) == ESP_OK);
    ok &= (nvs_set_i32(handle, "data_bits", config.data_bits) == ESP_OK);
    ok &= (nvs_set_i32(handle, "stop_bits", config.stop_bits) == ESP_OK);
    ok &= (nvs_set_i32(handle, "parity", config.parity) == ESP_OK);
    ok &= (nvs_set_i32(handle, "hex_rx", config.hex_rx ? 1 : 0) == ESP_OK);
    ok &= (nvs_set_i32(handle, "hex_tx", config.hex_tx ? 1 : 0) == ESP_OK);
    ok &= (nvs_commit(handle) == ESP_OK);
    nvs_close(handle);

    ESP_UTILS_CHECK_FALSE_RETURN(ok, false, "NVS write failed");
    return true;
}

} // namespace esp_brookesia::apps
