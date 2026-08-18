#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

class EthernetTest : public systems::phone::App {
public:
    static EthernetTest *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~EthernetTest();

protected:
    EthernetTest(bool use_status_bar, bool use_navigation_bar);

    bool init(void) override;
    bool deinit(void) override;
    bool run(void) override;
    bool back(void) override;
    bool close(void) override;
    bool pause(void) override;
    bool resume(void) override;
    bool cleanResource(void) override;

private:
    static constexpr size_t NETWORK_VALUE_COUNT = 6;
    static constexpr size_t TRAFFIC_VALUE_COUNT = 2;

    void createUi(void);
    void createNetworkSection(lv_obj_t *parent);
    void createTrafficSection(lv_obj_t *parent);
    void createConnectionSection(lv_obj_t *parent);
    void refreshInfo(void);
    void refreshPingResult(void);
    void startPing(void);

    static lv_obj_t *createSectionTitle(lv_obj_t *parent, const char *text);
    static lv_obj_t *createPanel(lv_obj_t *parent, int height);
    static lv_obj_t *createInfoRow(lv_obj_t *parent, const char *name, lv_obj_t **value_label);
    static void refreshTimerCallback(lv_timer_t *timer);
    static void pingButtonEventCallback(lv_event_t *event);

    static EthernetTest *_instance;

    esp_err_t _init_result = ESP_ERR_INVALID_STATE;
    bool _page_active = false;
    lv_obj_t *_root = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_obj_t *_network_values[NETWORK_VALUE_COUNT] = {};
    lv_obj_t *_traffic_values[TRAFFIC_VALUE_COUNT] = {};
    lv_obj_t *_ping_button = nullptr;
    lv_obj_t *_latency_label = nullptr;
    lv_obj_t *_ping_status_label = nullptr;
    lv_timer_t *_refresh_timer = nullptr;

    std::atomic_bool _ping_running = false;
    std::atomic_bool _ping_result_ready = false;
    std::atomic<int> _ping_result = ESP_ERR_INVALID_STATE;
    std::atomic<uint32_t> _ping_latency = 0;
};

} // namespace esp_brookesia::apps
