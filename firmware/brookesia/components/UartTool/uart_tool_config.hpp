#pragma once

#include "driver/uart.h"

namespace esp_brookesia::apps {

struct UartToolConfig {
    int tx_pin = 20;
    int rx_pin = 21;
    int baud = 115200;
    uart_word_length_t data_bits = UART_DATA_8_BITS;
    uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
    uart_parity_t parity = UART_PARITY_DISABLE;
    bool hex_rx = false;
    bool hex_tx = false;
};

UartToolConfig uart_tool_config_load();
bool uart_tool_config_save(const UartToolConfig &config);

} // namespace esp_brookesia::apps
