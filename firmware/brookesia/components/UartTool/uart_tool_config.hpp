#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "driver/uart.h"

namespace esp_brookesia::apps {

enum class UartToolMode : int32_t {
    Uart = 0,
    Rs485,
    Rs232,
};

struct UartToolFixedPins {
    gpio_num_t tx;
    gpio_num_t rx;
    gpio_num_t direction;
};

// Fixed routing on the ESP32-P4-NANO-WIFI6-DB board. UART mode uses GPIO21/GPIO22 on
// the P1 header (pins 15 and 16), which the board leaves free and no Board Manager
// peripheral claims. UART0 (GPIO37/GPIO38) is the console and must stay untouched.
// The board carries no RS485/RS232 transceiver, so those modes stay unavailable (NC)
// and their buttons are hidden in the UI.
constexpr UartToolFixedPins UART_TOOL_UART_PINS = {
    GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_NC
};
constexpr UartToolFixedPins UART_TOOL_RS485_PINS = {
    GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
};
constexpr UartToolFixedPins UART_TOOL_RS232_PINS = {
    GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
};

constexpr UartToolFixedPins uart_tool_fixed_pins(UartToolMode mode)
{
    switch (mode) {
    case UartToolMode::Rs485:
        return UART_TOOL_RS485_PINS;
    case UartToolMode::Rs232:
        return UART_TOOL_RS232_PINS;
    case UartToolMode::Uart:
    default:
        return UART_TOOL_UART_PINS;
    }
}

constexpr const char *uart_tool_mode_name(UartToolMode mode)
{
    switch (mode) {
    case UartToolMode::Rs485:
        return "RS485";
    case UartToolMode::Rs232:
        return "RS232";
    case UartToolMode::Uart:
    default:
        return "UART";
    }
}

constexpr bool uart_tool_fixed_pins_ready(UartToolMode mode)
{
    const UartToolFixedPins pins = uart_tool_fixed_pins(mode);
    return (pins.tx != GPIO_NUM_NC) &&
           (pins.rx != GPIO_NUM_NC) &&
           ((mode != UartToolMode::Rs485) || (pins.direction != GPIO_NUM_NC));
}

struct UartToolConfig {
    UartToolMode mode = UartToolMode::Uart;
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
