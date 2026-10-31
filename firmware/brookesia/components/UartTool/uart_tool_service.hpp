#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>
#include "boost/thread.hpp"
#include "uart_tool_config.hpp"

namespace esp_brookesia::apps {

class UartToolService {
public:
    UartToolService() = default;
    ~UartToolService();

    UartToolService(const UartToolService &) = delete;
    UartToolService &operator=(const UartToolService &) = delete;

    bool start(const UartToolConfig &config);
    void stop();
    bool isRunning() const
    {
        return _running;
    }
    size_t readRx(uint8_t *out, size_t max_len);
    bool send(const uint8_t *data, size_t len);

    // Thread contract: readRx() may be called from any thread; start(), stop()
    // and send() must all be called from the same thread (the GUI thread here),
    // so send() can never race a concurrent stop()/uart_driver_delete().

private:
    void rxThreadLoop();
    void txThreadLoop();
    void pushRx(const uint8_t *data, size_t len);
    void releasePins();

    static constexpr size_t RING_SIZE = 8192;
    static constexpr size_t TX_QUEUE_MAX_BYTES = 16 * 1024;

    uint8_t *_ring = nullptr;      // PSRAM, guarded by _mutex
    size_t _ring_head = 0;         // write index
    size_t _ring_count = 0;        // bytes stored
    std::mutex _mutex;
    std::atomic<bool> _exit{false};
    std::atomic<bool> _running{false};
    bool _driver_installed = false; // GUI-thread state; retained when driver cleanup fails
    // Pins routed to UART_PORT by the last start(); released after the driver is deleted
    // so switching modes does not leave the previous transceiver connected.
    UartToolFixedPins _pins = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    boost::thread _rx_thread;

    std::deque<std::vector<uint8_t>> _tx_queue;
    size_t _tx_queued_bytes = 0;
    std::mutex _tx_mutex;
    std::condition_variable _tx_cv;
    boost::thread _tx_thread;
};

} // namespace esp_brookesia::apps
