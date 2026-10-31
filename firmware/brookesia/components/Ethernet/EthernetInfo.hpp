#pragma once

#include <atomic>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lvgl.h"
#include "esp_eth_driver.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

class EthernetInfo: public systems::phone::App {
public:
    static EthernetInfo *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~EthernetInfo();

protected:
    EthernetInfo(bool use_status_bar, bool use_navigation_bar);

    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;

private:
    static EthernetInfo *_instance;

    esp_eth_handle_t *_eth_handles = nullptr;
    uint8_t _eth_count = 0;
    esp_netif_t *_eth_netif = nullptr;
    esp_eth_netif_glue_handle_t _eth_glue = nullptr;
    esp_event_handler_instance_t _eth_event_instance = nullptr;
    std::atomic_bool _link_up = false;
    std::atomic_bool _init_task_running = false;
    SemaphoreHandle_t _init_done = nullptr;
    lv_obj_t *_root = nullptr;
    lv_obj_t *_status_dot = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_obj_t *_value_labels[6] = {};
    lv_timer_t *_refresh_timer = nullptr;
    esp_err_t _init_result = ESP_ERR_INVALID_STATE;

    void createUi();
    void destroyUi();
    esp_err_t initializeEthernet();
    bool startEthernetInitialization();
    static void ethernetInitTask(void *arg);
    esp_err_t registerEthernetEvents();
    void unregisterEthernetEvents();
    static void ethernetEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void refresh();

    static void refreshTimerCallback(lv_timer_t *timer);
    static void refreshButtonCallback(lv_event_t *event);
};

} // namespace esp_brookesia::apps
