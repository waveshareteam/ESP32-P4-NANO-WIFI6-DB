#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:EthernetTest"
#include "esp_lib_utils.h"

#include <cstdio>
#include <cstring>
#include <inttypes.h>

#include <boost/thread.hpp>

#include "EthernetTest.hpp"
#include "bsp_board_extra.h"
#include "esp_netif_ip_addr.h"
#include "systems/base/assets/esp_brookesia_base_assets.h"

#define APP_NAME                    "Ethernet"
#define REFRESH_PERIOD_MS           500
#define PANEL_COLOR                 0x20262C
#define TEXT_PRIMARY_COLOR          0xF2F4F5
#define TEXT_SECONDARY_COLOR        0xAAB4BF
#define TEXT_ACCENT_COLOR           0x46E6E6
#define TEXT_SUCCESS_COLOR          0x67D68A
#define TEXT_ERROR_COLOR            0xFF8A80
#define BUTTON_COLOR                0x78E9F0
#define APP_TITLE_FONT              (&esp_brookesia_font_maison_neue_book_24)
#define SECTION_TITLE_FONT          (&esp_brookesia_font_maison_neue_book_18)
#define STATUS_FONT                 (&esp_brookesia_font_maison_neue_book_16)
#define CONTENT_FONT                (&esp_brookesia_font_maison_neue_book_14)

namespace esp_brookesia::apps {

EthernetTest *EthernetTest::_instance = nullptr;

static void set_label_text(lv_obj_t *label, const char *text)
{
    if (label != nullptr) {
        lv_label_set_text(label, text);
    }
}

static void format_ip(char *buffer, size_t buffer_size, const esp_ip4_addr_t *address, bool valid)
{
    if (!valid || address == nullptr || address->addr == 0) {
        std::snprintf(buffer, buffer_size, "---");
        return;
    }
    std::snprintf(buffer, buffer_size, IPSTR, IP2STR(address));
}

static bool mac_is_valid(const uint8_t *mac)
{
    for (size_t i = 0; i < 6; i++) {
        if (mac[i] != 0) {
            return true;
        }
    }
    return false;
}

EthernetTest *EthernetTest::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new EthernetTest(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

EthernetTest::EthernetTest(bool use_status_bar, bool use_navigation_bar):
    App(APP_NAME, nullptr, true, use_status_bar, use_navigation_bar)
{
}

EthernetTest::~EthernetTest()
{
    ESP_UTILS_LOGD("Destroy(@0x%p)", this);
}

bool EthernetTest::init(void)
{
    _init_result = bsp_extra_ethernet_init();
    if (_init_result != ESP_OK) {
        ESP_UTILS_LOGW("Ethernet init failed: %s", esp_err_to_name(_init_result));
    }
    return true;
}

bool EthernetTest::deinit(void)
{
    return true;
}

bool EthernetTest::run(void)
{
    ESP_UTILS_LOGD("Run(@0x%p)", this);

    if (_init_result != ESP_OK) {
        _init_result = bsp_extra_ethernet_init();
    }

    if (_root != nullptr) {
        _page_active = true;
        if (_refresh_timer != nullptr) {
            lv_timer_resume(_refresh_timer);
        }
        refreshInfo();
        return true;
    }

    createUi();
    _page_active = true;
    refreshInfo();
    _refresh_timer = lv_timer_create(refreshTimerCallback, REFRESH_PERIOD_MS, this);
    return _refresh_timer != nullptr;
}

bool EthernetTest::back(void)
{
    ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
    return true;
}

bool EthernetTest::close(void)
{
    ESP_UTILS_LOGD("Close(@0x%p)", this);
    _page_active = false;
    if (_refresh_timer != nullptr) {
        lv_timer_pause(_refresh_timer);
    }
    return true;
}

bool EthernetTest::pause(void)
{
    ESP_UTILS_LOGD("Pause(@0x%p)", this);
    _page_active = false;
    if (_refresh_timer != nullptr) {
        lv_timer_pause(_refresh_timer);
    }
    return true;
}

bool EthernetTest::resume(void)
{
    ESP_UTILS_LOGD("Resume(@0x%p)", this);
    _page_active = true;
    if (_refresh_timer != nullptr) {
        lv_timer_resume(_refresh_timer);
    }
    refreshInfo();
    return true;
}

bool EthernetTest::cleanResource(void)
{
    ESP_UTILS_LOGD("CleanResource(@0x%p)", this);
    _page_active = false;
    _root = nullptr;
    _status_label = nullptr;
    for (size_t i = 0; i < NETWORK_VALUE_COUNT; i++) {
        _network_values[i] = nullptr;
    }
    for (size_t i = 0; i < TRAFFIC_VALUE_COUNT; i++) {
        _traffic_values[i] = nullptr;
    }
    _ping_button = nullptr;
    _latency_label = nullptr;
    _ping_status_label = nullptr;
    _refresh_timer = nullptr;
    return true;
}

lv_obj_t *EthernetTest::createSectionTitle(lv_obj_t *parent, const char *text)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, text);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_height(title, 28);
    lv_obj_set_style_text_color(title, lv_color_hex(TEXT_ACCENT_COLOR), 0);
    lv_obj_set_style_text_font(title, SECTION_TITLE_FONT, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_pad_top(title, 2, 0);
    return title;
}

lv_obj_t *EthernetTest::createPanel(lv_obj_t *parent, int height)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, LV_PCT(100), height);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_pad_row(panel, 0, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

lv_obj_t *EthernetTest::createInfoRow(lv_obj_t *parent, const char *name, lv_obj_t **value_label)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 28);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_obj_set_width(name_label, LV_PCT(30));
    lv_obj_set_style_text_font(name_label, CONTENT_FONT, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(TEXT_PRIMARY_COLOR), 0);

    lv_obj_t *value = lv_label_create(row);
    lv_label_set_text(value, "---");
    lv_obj_set_width(value, LV_PCT(70));
    lv_label_set_long_mode(value, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(value, CONTENT_FONT, 0);
    lv_obj_set_style_text_color(value, lv_color_hex(TEXT_PRIMARY_COLOR), 0);
    *value_label = value;
    return row;
}

void EthernetTest::createNetworkSection(lv_obj_t *parent)
{
    createSectionTitle(parent, "Network Infomation");
    lv_obj_t *panel = createPanel(parent, 6 * 28);
    createInfoRow(panel, "IP Address:", &_network_values[0]);
    createInfoRow(panel, "Gateway:", &_network_values[1]);
    createInfoRow(panel, "NETmask:", &_network_values[2]);
    createInfoRow(panel, "DNS:", &_network_values[3]);
    createInfoRow(panel, "MAC:", &_network_values[4]);
    createInfoRow(panel, "Speed:", &_network_values[5]);
}

void EthernetTest::createTrafficSection(lv_obj_t *parent)
{
    createSectionTitle(parent, "Traffic Statistics");
    lv_obj_t *panel = createPanel(parent, 2 * 28);
    createInfoRow(panel, "Rx Packets:", &_traffic_values[0]);
    createInfoRow(panel, "Tx Packets:", &_traffic_values[1]);
}

void EthernetTest::createConnectionSection(lv_obj_t *parent)
{
    createSectionTitle(parent, "Connetction Test:");
    lv_obj_t *panel = createPanel(parent, 164);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(panel, 12, 0);

    _ping_button = lv_button_create(panel);
    lv_obj_set_size(_ping_button, 200, 52);
    lv_obj_set_style_bg_color(_ping_button, lv_color_hex(BUTTON_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_ping_button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(_ping_button, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(_ping_button, pingButtonEventCallback, LV_EVENT_CLICKED, this);
    lv_obj_t *button_label = lv_label_create(_ping_button);
    lv_label_set_text(button_label, "Ping Gateway");
    lv_obj_set_style_text_font(button_label, STATUS_FONT, 0);
    lv_obj_set_style_text_color(button_label, lv_color_hex(0x183238), 0);
    lv_obj_center(button_label);

    _latency_label = lv_label_create(panel);
    lv_label_set_text(_latency_label, "Latency:-ms");
    lv_obj_set_width(_latency_label, LV_PCT(100));
    lv_obj_set_style_text_align(_latency_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(_latency_label, CONTENT_FONT, 0);
    lv_obj_set_style_text_color(_latency_label, lv_color_hex(TEXT_PRIMARY_COLOR), 0);

    _ping_status_label = lv_label_create(panel);
    lv_label_set_text(_ping_status_label, "status:idle");
    lv_obj_set_width(_ping_status_label, LV_PCT(100));
    lv_obj_set_style_text_align(_ping_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(_ping_status_label, CONTENT_FONT, 0);
    lv_obj_set_style_text_color(_ping_status_label, lv_color_hex(TEXT_SECONDARY_COLOR), 0);
}

void EthernetTest::createUi(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x14181C), 0);

    _root = lv_obj_create(screen);
    lv_obj_remove_style_all(_root);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(_root);
    lv_obj_set_style_pad_left(_root, 14, 0);
    lv_obj_set_style_pad_right(_root, 14, 0);
    lv_obj_set_style_pad_top(_root, 8, 0);
    lv_obj_set_style_pad_bottom(_root, 14, 0);
    lv_obj_set_style_pad_row(_root, 4, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *title = lv_label_create(_root);
    lv_label_set_text(title, "Ethernet Network Test");
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_height(title, 36);
    lv_obj_set_style_text_color(title, lv_color_hex(TEXT_ACCENT_COLOR), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(title, APP_TITLE_FONT, 0);
    lv_obj_set_style_pad_bottom(title, 4, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(TEXT_ACCENT_COLOR), 0);
    lv_obj_set_style_border_width(title, 3, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);

    lv_obj_t *status_panel = createPanel(_root, 74);
    lv_obj_set_style_bg_color(status_panel, lv_color_hex(PANEL_COLOR), 0);
    lv_obj_set_style_bg_opa(status_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(status_panel, lv_color_hex(TEXT_ACCENT_COLOR), 0);
    lv_obj_set_style_border_width(status_panel, 2, 0);
    lv_obj_set_style_radius(status_panel, 8, 0);
    lv_obj_set_style_pad_all(status_panel, 12, 0);
    lv_obj_set_flex_align(status_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    _status_label = lv_label_create(status_panel);
    lv_label_set_text(_status_label, "Status:Disconnected");
    lv_obj_set_width(_status_label, LV_PCT(100));
    lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(_status_label, STATUS_FONT, 0);
    lv_obj_set_style_text_color(_status_label, lv_color_hex(TEXT_SECONDARY_COLOR), 0);

    createNetworkSection(_root);
    createTrafficSection(_root);
    createConnectionSection(_root);
}

void EthernetTest::refreshInfo(void)
{
    if (!_page_active || _root == nullptr) {
        return;
    }

    bsp_extra_ethernet_info_t info = {};
    esp_err_t ret = bsp_extra_ethernet_get_info(&info);
    const bool service_ready = (ret == ESP_OK && info.initialized);
    if (!service_ready) {
        set_label_text(_status_label, "Status:Init Failed");
        lv_obj_set_style_text_color(_status_label, lv_color_hex(TEXT_ERROR_COLOR), 0);
    } else {
        set_label_text(_status_label, info.link_up ? "Status:Connected" : "Status:Disconnected");
        lv_obj_set_style_text_color(
            _status_label,
            lv_color_hex(info.link_up ? TEXT_SUCCESS_COLOR : TEXT_SECONDARY_COLOR),
            0
        );
    }

    char buffer[40];
    format_ip(buffer, sizeof(buffer), &info.ip_info.ip, info.ip_acquired);
    set_label_text(_network_values[0], buffer);
    format_ip(buffer, sizeof(buffer), &info.ip_info.gw, info.ip_acquired);
    set_label_text(_network_values[1], buffer);
    format_ip(buffer, sizeof(buffer), &info.ip_info.netmask, info.ip_acquired);
    set_label_text(_network_values[2], buffer);

    if (info.ip_acquired && info.dns_main.ip.type == ESP_IPADDR_TYPE_V4) {
        format_ip(buffer, sizeof(buffer), &info.dns_main.ip.u_addr.ip4, true);
    } else {
        std::snprintf(buffer, sizeof(buffer), "---");
    }
    set_label_text(_network_values[3], buffer);

    if (service_ready && mac_is_valid(info.mac)) {
        std::snprintf(
            buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
            info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]
        );
    } else {
        std::snprintf(buffer, sizeof(buffer), "---");
    }
    set_label_text(_network_values[4], buffer);

    if (info.link_up && info.speed_mbps != 0) {
        std::snprintf(buffer, sizeof(buffer), "%u Mbps", (unsigned)info.speed_mbps);
    } else {
        std::snprintf(buffer, sizeof(buffer), "---");
    }
    set_label_text(_network_values[5], buffer);

    if (service_ready) {
        std::snprintf(buffer, sizeof(buffer), "%" PRIu32, info.rx_packets);
        set_label_text(_traffic_values[0], buffer);
        std::snprintf(buffer, sizeof(buffer), "%" PRIu32, info.tx_packets);
        set_label_text(_traffic_values[1], buffer);
    } else {
        set_label_text(_traffic_values[0], "---");
        set_label_text(_traffic_values[1], "---");
    }
}

void EthernetTest::refreshPingResult(void)
{
    if (!_ping_result_ready.exchange(false)) {
        return;
    }

    if (_ping_button != nullptr) {
        lv_obj_remove_state(_ping_button, LV_STATE_DISABLED);
    }

    esp_err_t result = (esp_err_t)_ping_result.load();
    if (result == ESP_OK) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "Latency:%" PRIu32 "ms", _ping_latency.load());
        set_label_text(_latency_label, buffer);
        set_label_text(_ping_status_label, "status:success");
        lv_obj_set_style_text_color(_ping_status_label, lv_color_hex(TEXT_SUCCESS_COLOR), 0);
    } else {
        set_label_text(_latency_label, "Latency:-ms");
        set_label_text(_ping_status_label, "status:failed");
        lv_obj_set_style_text_color(_ping_status_label, lv_color_hex(TEXT_ERROR_COLOR), 0);
    }
}

void EthernetTest::startPing(void)
{
    if (_ping_running.exchange(true)) {
        return;
    }

    _ping_result_ready.store(false);
    set_label_text(_latency_label, "Latency:-ms");
    set_label_text(_ping_status_label, "status:pinging");
    lv_obj_set_style_text_color(_ping_status_label, lv_color_hex(TEXT_ACCENT_COLOR), 0);
    lv_obj_add_state(_ping_button, LV_STATE_DISABLED);

    esp_utils::thread_config_guard thread_config({
        .name = "eth_ping",
        .stack_size = 4096,
    });
    boost::thread([this]() {
        uint32_t latency_ms = 0;
        esp_err_t result = bsp_extra_ethernet_ping_gateway(&latency_ms);
        _ping_latency.store(latency_ms);
        _ping_result.store((int)result);
        _ping_running.store(false);
        _ping_result_ready.store(true);
    }).detach();
}

void EthernetTest::refreshTimerCallback(lv_timer_t *timer)
{
    EthernetTest *self = static_cast<EthernetTest *>(lv_timer_get_user_data(timer));
    if (self == nullptr || !self->_page_active) {
        return;
    }
    self->refreshInfo();
    self->refreshPingResult();
}

void EthernetTest::pingButtonEventCallback(lv_event_t *event)
{
    EthernetTest *self = static_cast<EthernetTest *>(lv_event_get_user_data(event));
    if (self != nullptr) {
        self->startPing();
    }
}

} // namespace esp_brookesia::apps
