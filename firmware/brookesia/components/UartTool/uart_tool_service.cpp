#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:UartTool"
#include "esp_lib_utils.h"

#include <algorithm>
#include <cstring>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_tool_service.hpp"

#define UART_PORT           UART_NUM_1
#define UART_RX_BUF_SIZE    4096
#define RX_CHUNK_SIZE       256
#define RX_THREAD_STACK     4096
#define TX_THREAD_STACK     4096

// Override from the component/project compile definitions when lower idle wakeup
// frequency (for example 100 ms) is preferred over faster stop/reconfiguration.
#ifndef UART_TOOL_RX_READ_TIMEOUT_MS
#define UART_TOOL_RX_READ_TIMEOUT_MS 50
#endif

#ifndef UART_TOOL_TX_RETRY_DELAY_MS
#define UART_TOOL_TX_RETRY_DELAY_MS 5
#endif

namespace esp_brookesia::apps {

UartToolService::~UartToolService()
{
    stop();
}

bool UartToolService::start(const UartToolConfig &config)
{
    stop();
    if (_driver_installed) {
        ESP_UTILS_LOGE("Previous UART driver cleanup is incomplete");
        return false;
    }

    const UartToolFixedPins pins = uart_tool_fixed_pins(config.mode);
    if (!uart_tool_fixed_pins_ready(config.mode)) {
        ESP_UTILS_LOGW("%s pins are NC; UART service is not started", uart_tool_mode_name(config.mode));
        return false;
    }

    _ring = static_cast<uint8_t *>(heap_caps_malloc(RING_SIZE, MALLOC_CAP_SPIRAM));
    ESP_UTILS_CHECK_NULL_RETURN(_ring, false, "Alloc ring buffer failed");
    _ring_head = 0;
    _ring_count = 0;
    {
        std::lock_guard<std::mutex> lock(_tx_mutex);
        _tx_queue.clear();
        _tx_queued_bytes = 0;
    }

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
    _driver_installed = true;
    _pins = pins;

    err = uart_param_config(UART_PORT, &uart_config);
    if (err == ESP_OK) {
        err = uart_set_pin(
            UART_PORT,
            static_cast<int>(pins.tx),
            static_cast<int>(pins.rx),
            static_cast<int>(pins.direction),
            UART_PIN_NO_CHANGE
        );
    }
    if (err == ESP_OK) {
        const uart_mode_t uart_mode = (config.mode == UartToolMode::Rs485) ?
                                      UART_MODE_RS485_HALF_DUPLEX :
                                      UART_MODE_UART;
        err = uart_set_mode(UART_PORT, uart_mode);
    }
    if (err != ESP_OK) {
        ESP_UTILS_LOGE("uart config/set_pin/set_mode failed: %s", esp_err_to_name(err));
        const esp_err_t cleanup_err = uart_driver_delete(UART_PORT);
        if (cleanup_err == ESP_OK) {
            _driver_installed = false;
            releasePins();
        } else {
            ESP_UTILS_LOGE("Delete UART driver after configuration failure failed: %s",
                           esp_err_to_name(cleanup_err));
        }
        heap_caps_free(_ring);
        _ring = nullptr;
        return false;
    }

    // uart_set_pin() routes RX through the GPIO matrix without touching the pad's pull
    // resistors, so an unconnected header pin floats and clocks noise into the FIFO.
    // The internal pull-up holds the line at the UART idle level instead.
    const esp_err_t pull_err = gpio_pullup_en(pins.rx);
    if (pull_err != ESP_OK) {
        ESP_UTILS_LOGW("Pull up GPIO%d failed: %s", static_cast<int>(pins.rx),
                       esp_err_to_name(pull_err));
    }
    // Drop whatever the pad collected while it was still floating.
    const esp_err_t flush_err = uart_flush_input(UART_PORT);
    if (flush_err != ESP_OK) {
        ESP_UTILS_LOGW("Flush RX FIFO failed: %s", esp_err_to_name(flush_err));
    }

    _exit = false;
    {
        esp_utils::thread_config_guard thread_config({
            .name = "uart_tool_rx",
            .stack_size = RX_THREAD_STACK,
        });
        _rx_thread = boost::thread([this]() {
            rxThreadLoop();
        });
    }
    {
        esp_utils::thread_config_guard thread_config({
            .name = "uart_tool_tx",
            .stack_size = TX_THREAD_STACK,
        });
        _tx_thread = boost::thread([this]() {
            txThreadLoop();
        });
    }

    _running = true;
    ESP_UTILS_LOGI(
        "%s started: tx=%d rx=%d dir=%d baud=%d",
        uart_tool_mode_name(config.mode),
        static_cast<int>(pins.tx),
        static_cast<int>(pins.rx),
        static_cast<int>(pins.direction),
        config.baud
    );
    return true;
}

void UartToolService::stop()
{
    if (!_running && !_driver_installed) {
        return;
    }
    _exit = true;
    _tx_cv.notify_all();
    if (_rx_thread.joinable()) {
        _rx_thread.join();
    }
    if (_tx_thread.joinable()) {
        _tx_thread.join();
    }
    if (_driver_installed) {
        const esp_err_t delete_err = uart_driver_delete(UART_PORT);
        if (delete_err == ESP_OK) {
            _driver_installed = false;
            releasePins();
        } else {
            ESP_UTILS_LOGE("Delete UART driver failed: %s", esp_err_to_name(delete_err));
        }
    }
    {
        std::lock_guard<std::mutex> lock(_mutex);
        heap_caps_free(_ring);
        _ring = nullptr;
        _ring_head = 0;
        _ring_count = 0;
    }
    {
        std::lock_guard<std::mutex> lock(_tx_mutex);
        _tx_queue.clear();
        _tx_queued_bytes = 0;
    }
    _running = false;
    if (_driver_installed) {
        ESP_UTILS_LOGW("UART service stopped; driver cleanup is pending");
    } else {
        ESP_UTILS_LOGI("UART stopped");
    }
}

void UartToolService::releasePins()
{
    const gpio_num_t pins[] = {_pins.tx, _pins.rx, _pins.direction};
    for (const gpio_num_t pin : pins) {
        if (pin == GPIO_NUM_NC) {
            continue;
        }
        const esp_err_t err = gpio_reset_pin(pin);
        if (err != ESP_OK) {
            ESP_UTILS_LOGW("Reset GPIO%d failed: %s", static_cast<int>(pin), esp_err_to_name(err));
        }
    }
    // gpio_reset_pin() enables the pull-up. Pull the RS485 DE/RE line down instead
    // so the transceiver stays in receive mode and does not hold the bus.
    if (_pins.direction != GPIO_NUM_NC) {
        const esp_err_t err = gpio_set_pull_mode(_pins.direction, GPIO_PULLDOWN_ONLY);
        if (err != ESP_OK) {
            ESP_UTILS_LOGW("Pull down GPIO%d failed: %s", static_cast<int>(_pins.direction),
                           esp_err_to_name(err));
        }
    }
    _pins = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
}

void UartToolService::rxThreadLoop()
{
    uint8_t chunk[RX_CHUNK_SIZE];
    bool read_error_logged = false;
    while (!_exit) {
        const int bytes_read = uart_read_bytes(
            UART_PORT, chunk, sizeof(chunk), pdMS_TO_TICKS(UART_TOOL_RX_READ_TIMEOUT_MS)
        );
        if (bytes_read < 0) {
            if (!read_error_logged) {
                ESP_UTILS_LOGE("uart_read_bytes failed");
                read_error_logged = true;
            }
            vTaskDelay(1);
        } else {
            read_error_logged = false;
        }
        if (bytes_read > 0) {
            pushRx(chunk, static_cast<size_t>(bytes_read));
        }
    }
}

void UartToolService::txThreadLoop()
{
    while (true) {
        std::vector<uint8_t> data;
        {
            std::unique_lock<std::mutex> lock(_tx_mutex);
            _tx_cv.wait(lock, [this]() {
                return _exit || !_tx_queue.empty();
            });
            if (_exit) {
                return;
            }
            data = std::move(_tx_queue.front());
            _tx_queue.pop_front();
            _tx_queued_bytes -= data.size();
        }

        size_t offset = 0;
        while ((offset < data.size()) && !_exit) {
            const size_t remaining = data.size() - offset;
            const int written = uart_tx_chars(
                UART_PORT,
                reinterpret_cast<const char *>(data.data() + offset),
                static_cast<uint32_t>(remaining)
            );
            if (written < 0) {
                ESP_UTILS_LOGE("uart_tx_chars failed");
                break;
            }
            if (written > 0) {
                offset += static_cast<size_t>(written);
                continue;
            }
            vTaskDelay(std::max<TickType_t>(
                1, pdMS_TO_TICKS(UART_TOOL_TX_RETRY_DELAY_MS)
            ));
        }
    }
}

void UartToolService::pushRx(const uint8_t *data, size_t len)
{
    if (data == nullptr || len == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (_ring == nullptr) {
        return;
    }
    if (len >= RING_SIZE) {
        data += len - RING_SIZE;
        len = RING_SIZE;
    }
    const size_t first = std::min(len, RING_SIZE - _ring_head);
    memcpy(_ring + _ring_head, data, first);
    memcpy(_ring, data + first, len - first);
    _ring_head = (_ring_head + len) % RING_SIZE;
    _ring_count = std::min(RING_SIZE, _ring_count + len);
}

size_t UartToolService::readRx(uint8_t *out, size_t max_len)
{
    ESP_UTILS_CHECK_FALSE_RETURN(
        out != nullptr || max_len == 0,
        0,
        "Invalid RX destination"
    );
    if (max_len == 0) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (_ring == nullptr || _ring_count == 0) {
        return 0;
    }
    size_t n = std::min(max_len, _ring_count);
    size_t tail = (_ring_head + RING_SIZE - _ring_count) % RING_SIZE;
    const size_t first = std::min(n, RING_SIZE - tail);
    memcpy(out, _ring + tail, first);
    memcpy(out + first, _ring, n - first);
    _ring_count -= n;
    return n;
}

bool UartToolService::send(const uint8_t *data, size_t len)
{
    ESP_UTILS_CHECK_FALSE_RETURN(_running, false, "UART not running");
    ESP_UTILS_CHECK_FALSE_RETURN(data != nullptr || len == 0, false, "Invalid TX data");
    if (len == 0) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(_tx_mutex);
        ESP_UTILS_CHECK_FALSE_RETURN(!_exit, false, "UART is stopping");
        ESP_UTILS_CHECK_FALSE_RETURN(
            len <= (TX_QUEUE_MAX_BYTES - std::min(TX_QUEUE_MAX_BYTES, _tx_queued_bytes)),
            false,
            "TX queue full: queued=%zu new=%zu max=%zu",
            _tx_queued_bytes,
            len,
            TX_QUEUE_MAX_BYTES
        );
        _tx_queue.emplace_back(data, data + len);
        _tx_queued_bytes += len;
    }
    _tx_cv.notify_one();
    return true;
}

} // namespace esp_brookesia::apps
