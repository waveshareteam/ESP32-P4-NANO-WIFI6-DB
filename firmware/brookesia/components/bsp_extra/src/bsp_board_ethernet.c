/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp_board_extra.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_eth_phy_ip101.h"
#include "lwip/ip_addr.h"
#include "ping/ping_sock.h"
#include "sdkconfig.h"

#define BSP_EXTRA_ETH_PHY_ADDRESS       (1)
#define BSP_EXTRA_ETH_PHY_RESET_GPIO   (51)
#define BSP_EXTRA_ETH_MDC_GPIO         (31)
#define BSP_EXTRA_ETH_MDIO_GPIO        (52)
#define BSP_EXTRA_ETH_PING_DONE_BIT    (BIT0)

static const char *TAG = "bsp_extra_eth";

static portMUX_TYPE s_ethernet_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_ethernet_initializing;
static bool s_tx_rx_events_enabled;
static bsp_extra_ethernet_info_t s_ethernet_info = {
    .last_error = ESP_ERR_INVALID_STATE,
};
static esp_eth_mac_t *s_eth_mac;
static esp_eth_phy_t *s_eth_phy;
static esp_eth_handle_t s_eth_handle;
static esp_netif_t *s_eth_netif;
static esp_eth_netif_glue_handle_t s_eth_glue;
static esp_event_handler_instance_t s_eth_event_instance;
static esp_event_handler_instance_t s_eth_ip_event_instance;

static void ethernet_clear_ip_state(void)
{
    portENTER_CRITICAL(&s_ethernet_lock);
    s_ethernet_info.ip_acquired = false;
    memset(&s_ethernet_info.ip_info, 0, sizeof(s_ethernet_info.ip_info));
    memset(&s_ethernet_info.dns_main, 0, sizeof(s_ethernet_info.dns_main));
    portEXIT_CRITICAL(&s_ethernet_lock);
}

static void ethernet_refresh_mac(esp_eth_handle_t eth_handle)
{
    uint8_t mac[6] = {0};
    if (esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac) != ESP_OK) {
        return;
    }

    portENTER_CRITICAL(&s_ethernet_lock);
    memcpy(s_ethernet_info.mac, mac, sizeof(s_ethernet_info.mac));
    portEXIT_CRITICAL(&s_ethernet_lock);
}

static void ethernet_refresh_link_details(esp_eth_handle_t eth_handle)
{
    uint8_t mac[6] = {0};
    eth_speed_t speed = ETH_SPEED_10M;
    eth_duplex_t duplex = ETH_DUPLEX_HALF;
    esp_err_t mac_ret = esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac);
    esp_err_t speed_ret = esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
    esp_err_t duplex_ret = esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex);

    portENTER_CRITICAL(&s_ethernet_lock);
    if (mac_ret == ESP_OK) {
        memcpy(s_ethernet_info.mac, mac, sizeof(s_ethernet_info.mac));
    }
    if (speed_ret == ESP_OK) {
        s_ethernet_info.speed_mbps = (speed == ETH_SPEED_100M) ? 100 : 10;
    }
    if (duplex_ret == ESP_OK) {
        s_ethernet_info.full_duplex = (duplex == ETH_DUPLEX_FULL);
    }
    portEXIT_CRITICAL(&s_ethernet_lock);
}

static void ethernet_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_eth_handle_t eth_handle = s_eth_handle;
    if (event_data != NULL) {
        eth_handle = *(esp_eth_handle_t *)event_data;
    }

    switch (event_id) {
    case ETHERNET_EVENT_START:
        portENTER_CRITICAL(&s_ethernet_lock);
        s_ethernet_info.started = true;
        portEXIT_CRITICAL(&s_ethernet_lock);
        ethernet_refresh_mac(eth_handle);
        break;
    case ETHERNET_EVENT_STOP:
        portENTER_CRITICAL(&s_ethernet_lock);
        s_ethernet_info.started = false;
        s_ethernet_info.link_up = false;
        portEXIT_CRITICAL(&s_ethernet_lock);
        ethernet_clear_ip_state();
        break;
    case ETHERNET_EVENT_CONNECTED:
        portENTER_CRITICAL(&s_ethernet_lock);
        s_ethernet_info.link_up = true;
        portEXIT_CRITICAL(&s_ethernet_lock);
        ethernet_refresh_link_details(eth_handle);
        ESP_LOGI(TAG, "Ethernet link up");
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        portENTER_CRITICAL(&s_ethernet_lock);
        s_ethernet_info.link_up = false;
        s_ethernet_info.speed_mbps = 0;
        s_ethernet_info.full_duplex = false;
        portEXIT_CRITICAL(&s_ethernet_lock);
        ethernet_clear_ip_state();
        ESP_LOGI(TAG, "Ethernet link down");
        break;
    default:
        break;
    }
}

static void ethernet_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_data == NULL) {
        return;
    }

    if (event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_dns_info_t dns = {0};
        esp_err_t dns_ret = esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dns);

        portENTER_CRITICAL(&s_ethernet_lock);
        s_ethernet_info.ip_info = event->ip_info;
        s_ethernet_info.ip_acquired = true;
        if (dns_ret == ESP_OK) {
            s_ethernet_info.dns_main = dns;
        } else {
            memset(&s_ethernet_info.dns_main, 0, sizeof(s_ethernet_info.dns_main));
        }
        portEXIT_CRITICAL(&s_ethernet_lock);
        ESP_LOGI(TAG, "Ethernet got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        return;
    }

    if (event_id == IP_EVENT_ETH_LOST_IP) {
        ethernet_clear_ip_state();
        return;
    }

#if CONFIG_ESP_NETIF_REPORT_DATA_TRAFFIC
    if (event_id == IP_EVENT_TX_RX) {
        ip_event_tx_rx_t *event = (ip_event_tx_rx_t *)event_data;
        if (event->esp_netif != s_eth_netif) {
            return;
        }

        portENTER_CRITICAL(&s_ethernet_lock);
        if (event->dir == ESP_NETIF_RX) {
            s_ethernet_info.rx_packets++;
        } else {
            s_ethernet_info.tx_packets++;
        }
        portEXIT_CRITICAL(&s_ethernet_lock);
    }
#endif
}

static void ethernet_release_resources(void)
{
    if (s_eth_ip_event_instance != NULL) {
        esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, s_eth_ip_event_instance);
        s_eth_ip_event_instance = NULL;
    }
    if (s_eth_event_instance != NULL) {
        esp_event_handler_instance_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, s_eth_event_instance);
        s_eth_event_instance = NULL;
    }
#if CONFIG_ESP_NETIF_REPORT_DATA_TRAFFIC
    if (s_tx_rx_events_enabled && s_eth_netif != NULL) {
        esp_netif_tx_rx_event_disable(s_eth_netif);
    }
#endif
    s_tx_rx_events_enabled = false;
    if (s_eth_handle != NULL) {
        esp_eth_stop(s_eth_handle);
    }
    if (s_eth_glue != NULL) {
        esp_eth_del_netif_glue(s_eth_glue);
        s_eth_glue = NULL;
    }
    if (s_eth_netif != NULL) {
        esp_netif_destroy(s_eth_netif);
        s_eth_netif = NULL;
    }
    if (s_eth_handle != NULL) {
        esp_eth_driver_uninstall(s_eth_handle);
        s_eth_handle = NULL;
    }
    if (s_eth_mac != NULL) {
        s_eth_mac->del(s_eth_mac);
        s_eth_mac = NULL;
    }
    if (s_eth_phy != NULL) {
        s_eth_phy->del(s_eth_phy);
        s_eth_phy = NULL;
    }
}

esp_err_t bsp_extra_ethernet_init(void)
{
    portENTER_CRITICAL(&s_ethernet_lock);
    if (s_ethernet_info.initialized) {
        portEXIT_CRITICAL(&s_ethernet_lock);
        return ESP_OK;
    }
    if (s_ethernet_initializing) {
        portEXIT_CRITICAL(&s_ethernet_lock);
        return ESP_ERR_INVALID_STATE;
    }
    s_ethernet_initializing = true;
    portEXIT_CRITICAL(&s_ethernet_lock);

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        goto fail;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        goto fail;
    }

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = BSP_EXTRA_ETH_PHY_ADDRESS;
    phy_config.reset_gpio_num = BSP_EXTRA_ETH_PHY_RESET_GPIO;

    // The ESP32-P4 target macro supplies the board RMII data and clock pins.
    eth_esp32_emac_config_t emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_config.smi_gpio.mdc_num = BSP_EXTRA_ETH_MDC_GPIO;
    emac_config.smi_gpio.mdio_num = BSP_EXTRA_ETH_MDIO_GPIO;

    s_eth_mac = esp_eth_mac_new_esp32(&emac_config, &mac_config);
    if (s_eth_mac == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }

    s_eth_phy = esp_eth_phy_new_ip101(&phy_config);
    if (s_eth_phy == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(s_eth_mac, s_eth_phy);
    ret = esp_eth_driver_install(&eth_config, &s_eth_handle);
    if (ret != ESP_OK) {
        goto fail;
    }

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    s_eth_netif = esp_netif_new(&netif_config);
    if (s_eth_netif == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }

    s_eth_glue = esp_eth_new_netif_glue(s_eth_handle);
    if (s_eth_glue == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }

    ret = esp_netif_attach(s_eth_netif, s_eth_glue);
    if (ret != ESP_OK) {
        goto fail;
    }

#if CONFIG_ESP_NETIF_REPORT_DATA_TRAFFIC
    ret = esp_netif_tx_rx_event_enable(s_eth_netif);
    if (ret == ESP_OK) {
        s_tx_rx_events_enabled = true;
    } else {
        ESP_LOGW(TAG, "Traffic event reporting is unavailable: %s", esp_err_to_name(ret));
        ret = ESP_OK;
    }
#endif

    ret = esp_event_handler_instance_register(
        ETH_EVENT, ESP_EVENT_ANY_ID, ethernet_event_handler, NULL, &s_eth_event_instance
    );
    if (ret != ESP_OK) {
        goto fail;
    }
    ret = esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID, ethernet_ip_event_handler, NULL, &s_eth_ip_event_instance
    );
    if (ret != ESP_OK) {
        goto fail;
    }

    ret = esp_eth_start(s_eth_handle);
    if (ret != ESP_OK) {
        goto fail;
    }

    portENTER_CRITICAL(&s_ethernet_lock);
    s_ethernet_info.initialized = true;
    s_ethernet_info.last_error = ESP_OK;
    s_ethernet_initializing = false;
    portEXIT_CRITICAL(&s_ethernet_lock);
    return ESP_OK;

fail:
    ethernet_release_resources();
    portENTER_CRITICAL(&s_ethernet_lock);
    s_ethernet_info.initialized = false;
    s_ethernet_info.started = false;
    s_ethernet_info.link_up = false;
    s_ethernet_initializing = false;
    s_ethernet_info.last_error = ret;
    portEXIT_CRITICAL(&s_ethernet_lock);
    ESP_LOGE(TAG, "Ethernet initialization failed: %s", esp_err_to_name(ret));
    return ret;
}

esp_err_t bsp_extra_ethernet_get_info(bsp_extra_ethernet_info_t *info)
{
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_ethernet_lock);
    *info = s_ethernet_info;
    esp_err_t ret = s_ethernet_info.initialized ? ESP_OK : s_ethernet_info.last_error;
    portEXIT_CRITICAL(&s_ethernet_lock);
    return ret;
}

typedef struct {
    EventGroupHandle_t done;
    bool success;
    uint32_t latency_ms;
} gateway_ping_context_t;

static void gateway_ping_success_cb(esp_ping_handle_t handle, void *args)
{
    gateway_ping_context_t *context = (gateway_ping_context_t *)args;
    uint32_t time_ms = 0;
    if (esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &time_ms, sizeof(time_ms)) == ESP_OK) {
        context->latency_ms = time_ms;
        context->success = true;
    }
}

static void gateway_ping_end_cb(esp_ping_handle_t handle, void *args)
{
    gateway_ping_context_t *context = (gateway_ping_context_t *)args;
    xEventGroupSetBits(context->done, BSP_EXTRA_ETH_PING_DONE_BIT);
}

esp_err_t bsp_extra_ethernet_ping_gateway(uint32_t *latency_ms)
{
    if (latency_ms == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bsp_extra_ethernet_info_t info;
    esp_err_t ret = bsp_extra_ethernet_get_info(&info);
    if (ret != ESP_OK || !info.link_up || !info.ip_acquired || info.ip_info.gw.addr == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_t *eth_netif = NULL;
    portENTER_CRITICAL(&s_ethernet_lock);
    eth_netif = s_eth_netif;
    portEXIT_CRITICAL(&s_ethernet_lock);
    if (eth_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    gateway_ping_context_t context = {
        .done = xEventGroupCreate(),
        .success = false,
        .latency_ms = 0,
    };
    if (context.done == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count = 1;
    config.interval_ms = 100;
    config.timeout_ms = 1000;
    ip_addr_set_ip4_u32(&config.target_addr, info.ip_info.gw.addr);
    int netif_index = esp_netif_get_netif_impl_index(eth_netif);
    if (netif_index > 0) {
        config.interface = (uint32_t)netif_index;
    }

    esp_ping_callbacks_t callbacks = {
        .cb_args = &context,
        .on_ping_success = gateway_ping_success_cb,
        .on_ping_timeout = NULL,
        .on_ping_end = gateway_ping_end_cb,
    };
    esp_ping_handle_t ping_handle = NULL;
    ret = esp_ping_new_session(&config, &callbacks, &ping_handle);
    if (ret != ESP_OK) {
        vEventGroupDelete(context.done);
        return ret;
    }

    ret = esp_ping_start(ping_handle);
    if (ret == ESP_OK) {
        EventBits_t bits = xEventGroupWaitBits(
            context.done,
            BSP_EXTRA_ETH_PING_DONE_BIT,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(2500)
        );
        if ((bits & BSP_EXTRA_ETH_PING_DONE_BIT) == 0) {
            ret = ESP_ERR_TIMEOUT;
            esp_ping_stop(ping_handle);
        } else if (context.success) {
            *latency_ms = context.latency_ms;
        } else {
            ret = ESP_ERR_TIMEOUT;
        }
    }

    esp_ping_delete_session(ping_handle);
    vEventGroupDelete(context.done);
    return ret;
}
