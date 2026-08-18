#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
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
    void pushRx(const uint8_t *data, size_t len);

    static constexpr size_t RING_SIZE = 8192;

    uint8_t *_ring = nullptr;      // PSRAM, guarded by _mutex
    size_t _ring_head = 0;         // write index
    size_t _ring_count = 0;        // bytes stored
    std::mutex _mutex;
    std::atomic<bool> _exit{false};
    std::atomic<bool> _running{false};
    boost::thread _rx_thread;
};

} // namespace esp_brookesia::apps
