#pragma once

#include <string>
#include <vector>
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "uart_tool_config.hpp"
#include "uart_tool_service.hpp"

namespace esp_brookesia::apps {

class UartTool : public systems::phone::App {
public:
    static UartTool *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~UartTool();

protected:
    UartTool(bool use_status_bar, bool use_navigation_bar);

    bool run(void) override;
    bool back(void) override;
    bool close(void) override;
    bool pause(void) override;
    bool resume(void) override;
    bool cleanResource(void) override;

private:
    void createConfigSection(lv_obj_t *parent);
    void createRxSection(lv_obj_t *parent);
    void createTxSection(lv_obj_t *parent);
    void loadConfigToWidgets();
    void applyConfig();
    void setStatus(const char *text, bool is_error);
    void renderRxText();
    void appendRxBytes(const uint8_t *data, size_t len);
    void sendCurrentInput();

    static void apply_btn_event_cb(lv_event_t *e);
    static void rx_timer_cb(lv_timer_t *timer);
    static void rx_hex_switch_event_cb(lv_event_t *e);
    static void rx_clear_btn_event_cb(lv_event_t *e);
    static void tx_hex_switch_event_cb(lv_event_t *e);
    static void send_btn_event_cb(lv_event_t *e);
    static void tx_ta_event_cb(lv_event_t *e);

    UartToolConfig _config;
    UartToolService _service;

    // layout metrics derived from getVisualArea()
    int _content_w = 0;
    int _row_h = 0;

    lv_obj_t *_root = nullptr;
    lv_obj_t *_tx_pin_spinbox = nullptr;
    lv_obj_t *_rx_pin_spinbox = nullptr;
    lv_obj_t *_baud_dropdown = nullptr;
    lv_obj_t *_data_dropdown = nullptr;
    lv_obj_t *_stop_dropdown = nullptr;
    lv_obj_t *_parity_dropdown = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_obj_t *_rx_textarea = nullptr;
    lv_obj_t *_rx_hex_switch = nullptr;
    lv_obj_t *_tx_textarea = nullptr;
    lv_obj_t *_tx_hex_switch = nullptr;
    lv_obj_t *_keyboard = nullptr;

    std::vector<uint8_t> _rx_raw;   // capped raw history for re-render on hex toggle

    static UartTool *_instance;
};

} // namespace esp_brookesia::apps
