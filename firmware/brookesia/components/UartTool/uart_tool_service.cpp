#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:UartTool"
#include "esp_lib_utils.h"

#include <algorithm>
#include <cstring>
#include "esp_heap_caps.h"
#include "uart_tool_service.hpp"

#define UART_PORT           UART_NUM_1
#define UART_RX_BUF_SIZE    4096
#define RX_CHUNK_SIZE       256
#define RX_READ_TIMEOUT_MS  100
#define RX_THREAD_STACK     4096

namespace esp_brookesia::apps {

UartToolService::~UartToolService()
{
    stop();
}

bool UartToolService::start(const UartToolConfig &config)
{
    stop();

    _ring = static_cast<uint8_t *>(heap_caps_malloc(RING_SIZE, MALLOC_CAP_SPIRAM));
    ESP_UTILS_CHECK_NULL_RETURN(_ring, false, "Alloc ring buffer failed");
    _ring_head = 0;
    _ring_count = 0;

    uart_config_t uart_config = {};
    uart_config.baud_rate = config.baud;
    uart_config.data_bits = config.data_bits;
    uart_config.parity = config.parity;
    uart_config.stop_bits = config.stop_bits;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_driver_install(UART_PORT, UART_RX_BUF_SIZE, 0, 0, nullptr, 0);
    if (err != ESP_OK) {
        ESP_UTILS_LOGE("uart_driver_install failed: %s", esp_err_to_name(err));
        heap_caps_free(_ring);
        _ring = nullptr;
        return false;
    }

    err = uart_param_config(UART_PORT, &uart_config);
    if (err == ESP_OK) {
        err = uart_set_pin(UART_PORT, config.tx_pin, config.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (err != ESP_OK) {
        ESP_UTILS_LOGE("uart config/set_pin failed: %s", esp_err_to_name(err));
        uart_driver_delete(UART_PORT);
        heap_caps_free(_ring);
        _ring = nullptr;
        return false;
    }

    _exit = false;
    esp_utils::thread_config_guard thread_config({
        .name = "uart_tool_rx",
        .stack_size = RX_THREAD_STACK,
    });
    _rx_thread = boost::thread([this]() {
        rxThreadLoop();
    });

    _running = true;
    ESP_UTILS_LOGI("UART started: tx=%d rx=%d baud=%d", config.tx_pin, config.rx_pin, config.baud);
    return true;
}

void UartToolService::stop()
{
    if (!_running) {
        return;
    }
    _exit = true;
    if (_rx_thread.joinable()) {
        _rx_thread.join();
    }
    uart_driver_delete(UART_PORT);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        heap_caps_free(_ring);
        _ring = nullptr;
        _ring_head = 0;
        _ring_count = 0;
    }
    _running = false;
    ESP_UTILS_LOGI("UART stopped");
}

void UartToolService::rxThreadLoop()
{
    uint8_t chunk[RX_CHUNK_SIZE];
    while (!_exit) {
        int n = uart_read_bytes(UART_PORT, chunk, sizeof(chunk), pdMS_TO_TICKS(RX_READ_TIMEOUT_MS));
        if (n > 0) {
            pushRx(chunk, static_cast<size_t>(n));
        }
    }
}

void UartToolService::pushRx(const uint8_t *data, size_t len)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_ring == nullptr) {
        return;
    }
    if (len >= RING_SIZE) {
        data += len - RING_SIZE;
        len = RING_SIZE;
    }
    for (size_t i = 0; i < len; i++) {
        _ring[_ring_head] = data[i];
        _ring_head = (_ring_head + 1) % RING_SIZE;
        if (_ring_count < RING_SIZE) {
            _ring_count++;
        }
        // when full, head overwrites the oldest byte (tail implicitly advances)
    }
}

size_t UartToolService::readRx(uint8_t *out, size_t max_len)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_ring == nullptr || _ring_count == 0) {
        return 0;
    }
    size_t n = std::min(max_len, _ring_count);
    size_t tail = (_ring_head + RING_SIZE - _ring_count) % RING_SIZE;
    for (size_t i = 0; i < n; i++) {
        out[i] = _ring[(tail + i) % RING_SIZE];
    }
    _ring_count -= n;
    return n;
}

bool UartToolService::send(const uint8_t *data, size_t len)
{
    ESP_UTILS_CHECK_FALSE_RETURN(_running, false, "UART not running");
    int written = uart_write_bytes(UART_PORT, data, len);
    ESP_UTILS_CHECK_FALSE_RETURN(written == static_cast<int>(len), false, "uart_write_bytes wrote %d/%zu", written, len);
    return true;
}

} // namespace esp_brookesia::apps
