/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EthernetInfo.hpp"

#include <stdio.h>
#include <string.h>

#include "Ethernet.hpp"
#include "ethernet_init.h"
#include "esp_brookesia.hpp"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Ethernet"
#include "esp_lib_utils.h"

LV_IMG_DECLARE(img_app_ethernet);

namespace esp_brookesia::apps {

namespace {

lv_obj_t *addInfoRow(lv_obj_t *parent, const char *name, lv_obj_t **value_out)
{
    lv_obj_t *row = hardware_ui::create_panel(parent);
    lv_obj_set_size(row, lv_pct(100), 72);
    lv_obj_set_style_pad_hor(row, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(name_label, lv_color_hex(hardware_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);

    lv_obj_t *value_label = lv_label_create(row);
    lv_obj_set_width(value_label, lv_pct(64));
    lv_label_set_long_mode(value_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    hardware_ui::style_secondary_label(value_label, &lv_font_montserrat_20);
    lv_label_set_text(value_label, "--");
    *value_out = value_label;
    return row;
}

void setLabelIfChanged(lv_obj_t *label, const char *text)
{
    if (label && strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

} // namespace

EthernetInfo *EthernetInfo::_instance = nullptr;

EthernetInfo *EthernetInfo::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (!_instance) {
        _instance = new EthernetInfo(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

EthernetInfo::EthernetInfo(bool use_status_bar, bool use_navigation_bar):
    App(
        "Ethernet",
        &img_app_ethernet,
        true,
        use_status_bar,
        use_navigation_bar
    )
{
    _init_done = xSemaphoreCreateBinary();
}

EthernetInfo::~EthernetInfo()
{
    close();
    if (_init_done) {
        vSemaphoreDelete(_init_done);
        _init_done = nullptr;
    }
}

bool EthernetInfo::run()
{
    if (_eth_handles && _eth_count > 0) {
        _init_result = registerEthernetEvents();
    }
    createUi();
    refresh();
    return true;
}

bool EthernetInfo::back()
{
    return notifyCoreClosed();
}

bool EthernetInfo::close()
{
    unregisterEthernetEvents();
    destroyUi();
    return true;
}

bool EthernetInfo::init()
{
    return startEthernetInitialization();
}

bool EthernetInfo::deinit()
{
    destroyUi();
    if (_init_task_running.load() && _init_done) {
        xSemaphoreTake(_init_done, portMAX_DELAY);
    }
    if (_eth_handles) {
        unregisterEthernetEvents();
        esp_eth_stop(_eth_handles[0]);
        if (_eth_glue) {
            esp_eth_del_netif_glue(_eth_glue);
            _eth_glue = nullptr;
        }
        if (_eth_netif) {
            esp_netif_destroy(_eth_netif);
            _eth_netif = nullptr;
        }
        ethernet_deinit_all(_eth_handles);
        _eth_handles = nullptr;
        _eth_count = 0;
    }
    return true;
}

bool EthernetInfo::pause()
{
    unregisterEthernetEvents();
    if (_refresh_timer) {
        lv_timer_pause(_refresh_timer);
    }
    return true;
}

bool EthernetInfo::resume()
{
    if (_init_result == ESP_OK) {
        _init_result = registerEthernetEvents();
    }
    if (_refresh_timer) {
        lv_timer_resume(_refresh_timer);
    }
    return true;
}

void EthernetInfo::createUi()
{
    destroyUi();

    lv_obj_t *content = nullptr;
    _root = hardware_ui::create_page("Ethernet", &content);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, LV_PART_MAIN);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *status_panel = hardware_ui::create_panel(content);
    lv_obj_set_size(status_panel, lv_pct(100), 92);
    lv_obj_set_style_pad_hor(status_panel, 20, LV_PART_MAIN);
    lv_obj_set_flex_flow(status_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        status_panel,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_set_style_pad_column(status_panel, 16, LV_PART_MAIN);

    _status_dot = lv_obj_create(status_panel);
    lv_obj_set_size(_status_dot, 18, 18);
    lv_obj_set_style_radius(_status_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(_status_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(hardware_ui::COLOR_WARNING), LV_PART_MAIN);
    lv_obj_clear_flag(_status_dot, LV_OBJ_FLAG_SCROLLABLE);

    _status_label = lv_label_create(status_panel);
    lv_obj_set_flex_grow(_status_label, 1);
    lv_obj_set_style_text_font(_status_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_status_label, lv_color_hex(hardware_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
    lv_label_set_text(_status_label, "Starting");

    lv_obj_t *refresh_button = hardware_ui::create_icon_button(status_panel, LV_SYMBOL_REFRESH);
    lv_obj_add_event_cb(refresh_button, refreshButtonCallback, LV_EVENT_CLICKED, this);

    static const char *row_names[6] = {
        "IPv4 address",
        "Netmask",
        "Gateway",
        "MAC address",
        "Link",
        "PHY",
    };
    for (size_t i = 0; i < 6; ++i) {
        addInfoRow(content, row_names[i], &_value_labels[i]);
    }

    _refresh_timer = lv_timer_create(refreshTimerCallback, 500, this);
}

void EthernetInfo::destroyUi()
{
    if (_refresh_timer) {
        lv_timer_delete(_refresh_timer);
        _refresh_timer = nullptr;
    }
    if (_root) {
        lv_obj_delete(_root);
        _root = nullptr;
    }
    _status_dot = nullptr;
    _status_label = nullptr;
    for (auto &label : _value_labels) {
        label = nullptr;
    }
}

esp_err_t EthernetInfo::initializeEthernet()
{
    if (_eth_handles && _eth_count > 0) {
        return ESP_OK;
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = ethernet_init_all(&_eth_handles, &_eth_count);
    if (err != ESP_OK) {
        _eth_handles = nullptr;
        _eth_count = 0;
        return err;
    }
    if (_eth_count == 0 || !_eth_handles) {
        if (_eth_handles) {
            ethernet_deinit_all(_eth_handles);
            _eth_handles = nullptr;
        }
        _eth_count = 0;
        return ESP_ERR_NOT_FOUND;
    }

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    _eth_netif = esp_netif_new(&netif_config);
    if (!_eth_netif) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    _eth_glue = esp_eth_new_netif_glue(_eth_handles[0]);
    if (!_eth_glue) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    err = esp_netif_attach(_eth_netif, _eth_glue);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_eth_start(_eth_handles[0]);
    if (err != ESP_OK) {
        goto fail;
    }
    return ESP_OK;

fail:
    if (_eth_glue) {
        esp_eth_del_netif_glue(_eth_glue);
        _eth_glue = nullptr;
    }
    if (_eth_netif) {
        esp_netif_destroy(_eth_netif);
        _eth_netif = nullptr;
    }
    ethernet_deinit_all(_eth_handles);
    _eth_handles = nullptr;
    _eth_count = 0;
    return err;
}

bool EthernetInfo::startEthernetInitialization()
{
    if ((_eth_handles && _eth_count > 0) || _init_task_running.exchange(true)) {
        return true;
    }
    if (!_init_done) {
        _init_task_running.store(false);
        _init_result = ESP_ERR_NO_MEM;
        return false;
    }
    xSemaphoreTake(_init_done, 0);
    _init_result = ESP_ERR_INVALID_STATE;
    if (xTaskCreate(ethernetInitTask, "eth_init", 4096, this, 5, nullptr) != pdPASS) {
        _init_task_running.store(false);
        _init_result = ESP_ERR_NO_MEM;
        return false;
    }
    return true;
}

void EthernetInfo::ethernetInitTask(void *arg)
{
    auto *instance = static_cast<EthernetInfo *>(arg);
    if (!instance) {
        vTaskDelete(nullptr);
        return;
    }
    const esp_err_t result = instance->initializeEthernet();
    instance->_init_result = result;
    instance->_init_task_running.store(false);
    if (instance->_init_done) {
        xSemaphoreGive(instance->_init_done);
    }
    vTaskDelete(nullptr);
}

esp_err_t EthernetInfo::registerEthernetEvents()
{
    if (_eth_event_instance) {
        return ESP_OK;
    }
    _link_up.store(_eth_netif && esp_netif_is_netif_up(_eth_netif));
    return esp_event_handler_instance_register(
        ETH_EVENT,
        ESP_EVENT_ANY_ID,
        ethernetEventCallback,
        this,
        &_eth_event_instance
    );
}

void EthernetInfo::unregisterEthernetEvents()
{
    if (_eth_event_instance) {
        esp_event_handler_instance_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, _eth_event_instance);
        _eth_event_instance = nullptr;
    }
    _link_up.store(false);
}

void EthernetInfo::ethernetEventCallback(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    auto *instance = static_cast<EthernetInfo *>(arg);
    if (!instance || event_base != ETH_EVENT) {
        return;
    }
    if (event_id == ETHERNET_EVENT_CONNECTED) {
        instance->_link_up.store(true);
    } else if (event_id == ETHERNET_EVENT_DISCONNECTED || event_id == ETHERNET_EVENT_STOP) {
        instance->_link_up.store(false);
    }
}

void EthernetInfo::refresh()
{
    if (!_root) {
        return;
    }

    esp_netif_ip_info_t ip_info = {};
    const bool ip_acquired = _eth_netif &&
        esp_netif_get_ip_info(_eth_netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0;
    uint8_t mac[ETH_ADDR_LEN] = {};
    eth_speed_t speed = ETH_SPEED_10M;
    eth_duplex_t duplex = ETH_DUPLEX_HALF;
    uint32_t phy_addr = 0;
    if (_eth_handles && _eth_count > 0) {
        esp_eth_ioctl(_eth_handles[0], ETH_CMD_G_MAC_ADDR, mac);
        esp_eth_ioctl(_eth_handles[0], ETH_CMD_G_SPEED, &speed);
        esp_eth_ioctl(_eth_handles[0], ETH_CMD_G_DUPLEX_MODE, &duplex);
        esp_eth_ioctl(_eth_handles[0], ETH_CMD_G_PHY_ADDR, &phy_addr);
    }

    const bool initializing = _init_task_running.load();
    const char *status = initializing ? "Initializing" : "Starting";
    uint32_t status_color = hardware_ui::COLOR_WARNING;
    if (initializing) {
        status = "Initializing";
    } else if (_init_result != ESP_OK) {
        status = "Initialization error";
        status_color = hardware_ui::COLOR_ERROR;
    } else if (!_link_up.load()) {
        status = "Cable disconnected";
        status_color = hardware_ui::COLOR_SECONDARY_TEXT;
    } else if (!ip_acquired) {
        status = "Link up / DHCP";
        status_color = hardware_ui::COLOR_WARNING;
    } else {
        status = "Connected";
        status_color = hardware_ui::COLOR_SUCCESS;
    }
    setLabelIfChanged(_status_label, status);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(status_color), LV_PART_MAIN);

    char buffer[64];
    if (ip_acquired) {
        snprintf(buffer, sizeof(buffer), IPSTR, IP2STR(&ip_info.ip));
        setLabelIfChanged(_value_labels[0], buffer);
        snprintf(buffer, sizeof(buffer), IPSTR, IP2STR(&ip_info.netmask));
        setLabelIfChanged(_value_labels[1], buffer);
        snprintf(buffer, sizeof(buffer), IPSTR, IP2STR(&ip_info.gw));
        setLabelIfChanged(_value_labels[2], buffer);
    } else {
        setLabelIfChanged(_value_labels[0], "--");
        setLabelIfChanged(_value_labels[1], "--");
        setLabelIfChanged(_value_labels[2], "--");
    }

    snprintf(
        buffer,
        sizeof(buffer),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );
    setLabelIfChanged(_value_labels[3], buffer);

    if (_link_up.load()) {
        snprintf(
            buffer,
            sizeof(buffer),
            "%u Mbps / %s",
            speed == ETH_SPEED_100M ? 100U : 10U,
            duplex == ETH_DUPLEX_FULL ? "full duplex" : "half duplex"
        );
        setLabelIfChanged(_value_labels[4], buffer);
    } else {
        setLabelIfChanged(_value_labels[4], "--");
    }
    snprintf(buffer, sizeof(buffer), "IP101 / address %lu", (unsigned long)phy_addr);
    setLabelIfChanged(_value_labels[5], buffer);
}

void EthernetInfo::refreshTimerCallback(lv_timer_t *timer)
{
    auto *instance = static_cast<EthernetInfo *>(lv_timer_get_user_data(timer));
    if (instance) {
        if (!instance->_init_task_running.load() && instance->_init_result == ESP_OK &&
            !instance->_eth_event_instance && instance->_root) {
            instance->_init_result = instance->registerEthernetEvents();
        }
        instance->refresh();
    }
}

void EthernetInfo::refreshButtonCallback(lv_event_t *event)
{
    auto *instance = static_cast<EthernetInfo *>(lv_event_get_user_data(event));
    if (instance) {
        if (!instance->_eth_handles || instance->_eth_count == 0) {
            instance->startEthernetInitialization();
        } else {
            instance->_init_result = instance->registerEthernetEvents();
        }
        instance->refresh();
    }
}

// The app is a singleton owned by itself; the registry must not delete it.
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, EthernetInfo, "Ethernet", []()
{
    return std::shared_ptr<EthernetInfo>(EthernetInfo::requestInstance(), [](EthernetInfo *) {});
})

} // namespace esp_brookesia::apps
