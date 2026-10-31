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
    void createUi(lv_obj_t *screen, int width, int height);
    void rebuildUi(int width, int height);
    void deleteUiObjects();
    void clearUiPointers();
    void createHeader(lv_obj_t *parent);
    void createConnectionCard(lv_obj_t *parent);
    void createConfigSection(lv_obj_t *parent);
    void createRxSection(lv_obj_t *parent);
    void createTxSection(lv_obj_t *parent);
    void createKeyboard();
    void loadConfigToWidgets();
    void applyConfig();
    bool commitConfig(const UartToolConfig &config);
    void setStatus(const char *text, uint32_t color);
    void updateConnectionStatus();
    void updateConnectionSummary();
    void updateModeButtons();
    void renderRxText();
    void appendRxBytes(const uint8_t *data, size_t len);
    void sendCurrentInput();

    static void settings_btn_event_cb(lv_event_t *e);
    static void settings_close_btn_event_cb(lv_event_t *e);
    static void apply_btn_event_cb(lv_event_t *e);
    static void serial_mode_btn_event_cb(lv_event_t *e);
    static void rx_timer_cb(lv_timer_t *timer);
    static void rx_hex_switch_event_cb(lv_event_t *e);
    static void rx_clear_btn_event_cb(lv_event_t *e);
    static void tx_hex_switch_event_cb(lv_event_t *e);
    static void line_ending_btn_event_cb(lv_event_t *e);
    static void send_btn_event_cb(lv_event_t *e);
    static void tx_ta_event_cb(lv_event_t *e);
    static void screen_size_changed_event_cb(lv_event_t *e);
    static void layout_update_async_cb(void *user_data);

    UartToolConfig _config;
    UartToolService _service;

    // layout metrics derived from getVisualArea()
    int _content_w = 0;
    int _row_h = 0;
    bool _compact_layout = false;
    bool _narrow_layout = false;
    int _layout_w = 0;
    int _layout_h = 0;
    bool _layout_update_pending = false;

    lv_obj_t *_screen = nullptr;
    lv_obj_t *_root = nullptr;
    lv_obj_t *_status_dot = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_obj_t *_serial_mode_buttons[3] = {};
    lv_obj_t *_baud_summary_label = nullptr;
    lv_obj_t *_format_summary_label = nullptr;
    lv_obj_t *_settings_overlay = nullptr;
    lv_obj_t *_baud_dropdown = nullptr;
    lv_obj_t *_data_dropdown = nullptr;
    lv_obj_t *_stop_dropdown = nullptr;
    lv_obj_t *_parity_dropdown = nullptr;
    lv_obj_t *_rx_scroll = nullptr;
    lv_obj_t *_rx_label = nullptr;
    lv_obj_t *_rx_count_label = nullptr;
    lv_obj_t *_rx_ascii_mode_btn = nullptr;
    lv_obj_t *_rx_hex_switch = nullptr;
    lv_obj_t *_auto_scroll_checkbox = nullptr;
    lv_timer_t *_rx_timer = nullptr;
    lv_obj_t *_tx_textarea = nullptr;
    lv_obj_t *_tx_hex_switch = nullptr;
    lv_obj_t *_tx_text_mode_btn = nullptr;
    lv_obj_t *_line_ending_buttons[4] = {};
    lv_obj_t *_keyboard = nullptr;

    uint8_t _line_ending = 0;  // 0=None, 1=CR, 2=LF, 3=CRLF
    std::vector<uint8_t> _rx_raw;   // capped raw history for re-render on hex toggle
    std::string _rx_render_text;     // reused to avoid allocating every refresh

    static UartTool *_instance;
};

} // namespace esp_brookesia::apps
