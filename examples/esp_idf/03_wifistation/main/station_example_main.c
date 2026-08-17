/* WiFi SoftAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "ping/ping_sock.h"

#define EXAMPLE_WIFI_SSID       CONFIG_ESP_WIFI_SSID
#define EXAMPLE_WIFI_PASSWORD   CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_WIFI_CHANNEL    CONFIG_ESP_WIFI_AP_CHANNEL
#define EXAMPLE_MAX_CONNECTIONS CONFIG_ESP_WIFI_AP_MAX_CONNECTIONS

static const char *TAG = "wifi station";

#if CONFIG_ESP_WIFI_SOFTAP_MODE

static void log_station_event(const char *event_name, const uint8_t mac[6], int aid)
{
    ESP_LOGI(TAG, "%s: %02X:%02X:%02X:%02X:%02X:%02X, aid=%d",
             event_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], aid);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base != WIFI_EVENT) {
        return;
    }

    if (event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "SoftAP started");
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        log_station_event("Station connected", event->mac, event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        log_station_event("Station disconnected", event->mac, event->aid);
    }
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif != NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.ap.ssid, EXAMPLE_WIFI_SSID,
            sizeof(wifi_config.ap.ssid));
    strlcpy((char *)wifi_config.ap.password, EXAMPLE_WIFI_PASSWORD,
            sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(EXAMPLE_WIFI_SSID);
    wifi_config.ap.channel = EXAMPLE_WIFI_CHANNEL;
    wifi_config.ap.max_connection = EXAMPLE_MAX_CONNECTIONS;
    wifi_config.ap.authmode = strlen(EXAMPLE_WIFI_PASSWORD) == 0
                              ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(ap_netif, &ip_info));
    ESP_LOGI(TAG, "SoftAP ready");
    ESP_LOGI(TAG, "SSID: %s", EXAMPLE_WIFI_SSID);
    ESP_LOGI(TAG, "Password: %s", strlen(EXAMPLE_WIFI_PASSWORD) == 0 ? "<open>" : EXAMPLE_WIFI_PASSWORD);
    ESP_LOGI(TAG, "Channel: %d, max connections: %d", EXAMPLE_WIFI_CHANNEL, EXAMPLE_MAX_CONNECTIONS);
    ESP_LOGI(TAG, "AP IP address: " IPSTR, IP2STR(&ip_info.ip));
}

#else

#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_PING_DONE_BIT BIT0
#define EXAMPLE_PING_COUNT 100

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num;
static EventGroupHandle_t s_ping_event_group;
static esp_ip4_addr_t s_gateway_ip;

static void ping_on_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted = 0, received = 0, duration_ms = 0;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &duration_ms, sizeof(duration_ms));
    uint32_t lost = transmitted > received ? transmitted - received : 0;
    uint32_t loss_percent = transmitted ? (lost * 100U) / transmitted : 100U;
    ESP_LOGI(TAG, "Gateway ping finished: transmitted=%" PRIu32 ", received=%" PRIu32
             ", lost=%" PRIu32 ", loss=%" PRIu32 "%%, duration=%" PRIu32 " ms",
             transmitted, received, lost, loss_percent, duration_ms);
    xEventGroupSetBits(s_ping_event_group, WIFI_PING_DONE_BIT);
}

static esp_err_t ping_gateway(void)
{
    s_ping_event_group = xEventGroupCreate();
    if (s_ping_event_group == NULL) return ESP_ERR_NO_MEM;
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count = EXAMPLE_PING_COUNT;
    config.interval_ms = 100;
    config.timeout_ms = 1000;
    ip_addr_set_ip4_u32(&config.target_addr, s_gateway_ip.addr);
    esp_ping_callbacks_t callbacks = {
        .cb_args = NULL, .on_ping_success = NULL, .on_ping_timeout = NULL,
        .on_ping_end = ping_on_end,
    };
    esp_ping_handle_t ping = NULL;
    esp_err_t ret = esp_ping_new_session(&config, &callbacks, &ping);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Pinging gateway " IPSTR " (%d packets)", IP2STR(&s_gateway_ip), EXAMPLE_PING_COUNT);
        ret = esp_ping_start(ping);
    }
    if (ret == ESP_OK) {
        xEventGroupWaitBits(s_ping_event_group, WIFI_PING_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
        ret = esp_ping_delete_session(ping);
        ping = NULL;
    }
    if (ping != NULL) esp_ping_delete_session(ping);
    vEventGroupDelete(s_ping_event_group);
    s_ping_event_group = NULL;
    return ret;
}

static void sta_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = event_data;
        s_gateway_ip = event->ip_info.gw;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "gateway:" IPSTR, IP2STR(&s_gateway_ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id, instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &sta_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &sta_event_handler, NULL, &instance_got_ip));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASSWORD,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", EXAMPLE_WIFI_SSID, EXAMPLE_WIFI_PASSWORD);
        if (ping_gateway() != ESP_OK) ESP_LOGE(TAG, "Gateway ping test failed to start or complete");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", EXAMPLE_WIFI_SSID, EXAMPLE_WIFI_PASSWORD);
    }
}

#endif

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if CONFIG_ESP_WIFI_SOFTAP_MODE
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
#else
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
#endif
}
