/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_bit_defs.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_hosted.h"
#include "esp_hosted_bluedroid.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define ADV_CONFIG_FLAG      BIT0
#define SCAN_RSP_CONFIG_FLAG BIT1
#define GATTS_APP_ID         0x55
#define SVC_INST_ID          0
#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))
#define GATTS_CHAR_VAL_MAX_LEN 128

static const char *TAG = "bt_ctrl_mac";
static const char *DEVICE_NAME = "P4_GATTS_DEMO";

enum {
    IDX_SVC,
    IDX_CHAR,
    IDX_CHAR_VAL,
    IDX_CHAR_CFG,
    GATTS_IDX_NB,
};

static uint8_t s_adv_config_done;
static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x20,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t s_adv_raw_data[] = {
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xFF, 0x00,
    0x0E, ESP_BLE_AD_TYPE_NAME_CMPL, 'P', '4', '_', 'G', 'A', 'T', 'T', 'S', '_', 'D', 'E', 'M', 'O',
    0x02, ESP_BLE_AD_TYPE_TX_PWR, 0x09,
};

static uint8_t s_scan_rsp_raw_data[] = {
    0x08, ESP_BLE_AD_TYPE_LE_DEV_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x03, ESP_BLE_AD_TYPE_APPEARANCE, 0x00, 0x02,
};

static const uint16_t s_service_uuid = 0x00FF;
static const uint16_t s_char_uuid = 0xFF01;
static const uint16_t s_primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t s_character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t s_character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t s_char_properties = ESP_GATT_CHAR_PROP_BIT_READ |
                                         ESP_GATT_CHAR_PROP_BIT_WRITE |
                                         ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static uint8_t s_ccc_value[2] = { 0x00, 0x00 };
static uint8_t s_char_value[GATTS_CHAR_VAL_MAX_LEN] = "hello from esp32-p4";
static uint16_t s_char_value_len = sizeof("hello from esp32-p4") - 1;
static uint16_t s_handle_table[GATTS_IDX_NB];
static uint16_t s_conn_id;
static bool s_is_connected;
static bool s_notify_enabled;

static const esp_gatts_attr_db_t s_gatt_db[GATTS_IDX_NB] = {
    [IDX_SVC] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&s_primary_service_uuid, ESP_GATT_PERM_READ,
          sizeof(uint16_t), sizeof(s_service_uuid), (uint8_t *)&s_service_uuid}},

    [IDX_CHAR] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&s_character_declaration_uuid, ESP_GATT_PERM_READ,
          CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&s_char_properties}},

    [IDX_CHAR_VAL] =
        {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&s_char_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
          GATTS_CHAR_VAL_MAX_LEN, sizeof("hello from esp32-p4") - 1, s_char_value}},

    [IDX_CHAR_CFG] =
        {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&s_character_client_config_uuid,
          ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(s_ccc_value), s_ccc_value}},
};

static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

#if CONFIG_EXAMPLE_UPDATE_BT_MAC_ADDRESS
static esp_err_t parse_mac_address(const char *text, uint8_t mac[6])
{
    unsigned int values[6];
    int parsed = sscanf(text, "%02x:%02x:%02x:%02x:%02x:%02x",
                        &values[0], &values[1], &values[2],
                        &values[3], &values[4], &values[5]);
    if (parsed != 6) {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; i < 6; i++) {
        if (values[i] > 0xff) {
            return ESP_ERR_INVALID_ARG;
        }
        mac[i] = (uint8_t)values[i];
    }
    return ESP_OK;
}
#endif

static void log_coprocessor_info(void)
{
    esp_hosted_coprocessor_fwver_t fwver;
    esp_err_t ret = esp_hosted_get_coprocessor_fwversion(&fwver);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "co-processor firmware: %" PRIu32 ".%" PRIu32 ".%" PRIu32,
                 fwver.major1, fwver.minor1, fwver.patch1);
    } else {
        ESP_LOGW(TAG, "failed to get co-processor firmware version: %s", esp_err_to_name(ret));
    }

    char cp_name[30] = { 0 };
    uint32_t cp_chip_id = 0;
    ret = esp_hosted_get_cp_info(&cp_chip_id, cp_name, sizeof(cp_name));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "co-processor chip id: 0x%08" PRIx32 ", name: %s", cp_chip_id, cp_name);
    } else {
        ESP_LOGW(TAG, "failed to get co-processor info: %s", esp_err_to_name(ret));
    }
}

static esp_err_t read_bt_mac(uint8_t mac[6])
{
    size_t mac_len = esp_hosted_iface_mac_addr_len_get(ESP_MAC_BT);
    if (mac_len != 6) {
        ESP_LOGE(TAG, "unexpected BT MAC length: %u", (unsigned int)mac_len);
        return ESP_FAIL;
    }

    esp_err_t ret = esp_hosted_iface_mac_addr_get(mac, mac_len, ESP_MAC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to read BT controller MAC: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "BT controller MAC: " MACSTR, MAC2STR(mac));
    return ESP_OK;
}

static esp_err_t maybe_update_bt_mac(void)
{
    uint8_t mac[6] = { 0 };
    ESP_RETURN_ON_ERROR(read_bt_mac(mac), TAG, "BT MAC read failed");

#if CONFIG_EXAMPLE_UPDATE_BT_MAC_ADDRESS
    ESP_RETURN_ON_ERROR(parse_mac_address(CONFIG_EXAMPLE_BT_MAC_ADDRESS, mac),
                        TAG, "invalid configured BT MAC address");

    ESP_LOGI(TAG, "updating BT controller MAC to " MACSTR, MAC2STR(mac));
    ESP_RETURN_ON_ERROR(esp_hosted_iface_mac_addr_set(mac, sizeof(mac), ESP_MAC_BT),
                        TAG, "failed to update BT controller MAC");
    ESP_RETURN_ON_ERROR(read_bt_mac(mac), TAG, "failed to read updated BT MAC");
#else
    ESP_LOGI(TAG, "BT MAC update disabled; enable it in menuconfig if needed");
#endif

    return ESP_OK;
}

static esp_err_t configure_ble_security(void)
{
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;

    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE,
                                                       &auth_req, sizeof(auth_req)),
                        TAG, "failed to set BLE auth request mode");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE,
                                                       &iocap, sizeof(iocap)),
                        TAG, "failed to set BLE IO capability");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE,
                                                       &key_size, sizeof(key_size)),
                        TAG, "failed to set BLE key size");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH,
                                                       &auth_option, sizeof(auth_option)),
                        TAG, "failed to set BLE auth option");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT,
                                                       &oob_support, sizeof(oob_support)),
                        TAG, "failed to set BLE OOB support");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY,
                                                       &init_key, sizeof(init_key)),
                        TAG, "failed to set BLE init key mask");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY,
                                                       &rsp_key, sizeof(rsp_key)),
                        TAG, "failed to set BLE response key mask");

    ESP_LOGI(TAG, "BLE security configured for Just Works bonding");
    return ESP_OK;
}

static void send_attr_read_response(esp_gatt_if_t gatts_if, uint16_t conn_id, uint32_t trans_id,
                                    uint16_t handle, const uint8_t *value, uint16_t value_len,
                                    uint16_t offset)
{
    esp_gatt_rsp_t rsp = { 0 };
    esp_gatt_status_t status = ESP_GATT_OK;
    uint16_t rsp_len = 0;

    if (offset > value_len) {
        status = ESP_GATT_INVALID_OFFSET;
    } else {
        rsp_len = value_len - offset;
        if (rsp_len > sizeof(rsp.attr_value.value)) {
            rsp_len = sizeof(rsp.attr_value.value);
        }
        if (rsp_len > 0) {
            memcpy(rsp.attr_value.value, value + offset, rsp_len);
        }
    }

    rsp.attr_value.handle = handle;
    rsp.attr_value.len = rsp_len;
    rsp.attr_value.offset = offset;
    rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;

    esp_err_t ret = esp_ble_gatts_send_response(gatts_if, conn_id, trans_id, status, &rsp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to send read response: %s", esp_err_to_name(ret));
    }
}

static void handle_gatts_read(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "GATT read, conn_id=%u, handle=%u, offset=%u",
             param->read.conn_id, param->read.handle, param->read.offset);

    if (param->read.handle == s_handle_table[IDX_CHAR_VAL]) {
        send_attr_read_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                param->read.handle, s_char_value, s_char_value_len,
                                param->read.offset);
    } else if (param->read.handle == s_handle_table[IDX_CHAR_CFG]) {
        send_attr_read_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                param->read.handle, s_ccc_value, sizeof(s_ccc_value),
                                param->read.offset);
    } else {
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_INVALID_HANDLE, NULL);
    }
}

static void handle_gatts_write(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;

    if (param->write.is_prep) {
        status = ESP_GATT_INVALID_ATTR_LEN;
    } else if (param->write.handle == s_handle_table[IDX_CHAR_CFG] && param->write.len == sizeof(s_ccc_value)) {
        memcpy(s_ccc_value, param->write.value, sizeof(s_ccc_value));
        uint16_t ccc = ((uint16_t)s_ccc_value[1] << 8) | s_ccc_value[0];
        s_notify_enabled = (ccc & 0x0001) != 0;
        ESP_LOGI(TAG, "notifications %s", s_notify_enabled ? "enabled" : "disabled");
        esp_ble_gatts_set_attr_value(s_handle_table[IDX_CHAR_CFG], sizeof(s_ccc_value), s_ccc_value);
    } else if (param->write.handle == s_handle_table[IDX_CHAR_VAL]) {
        uint16_t copy_len = param->write.len;
        if (copy_len > sizeof(s_char_value)) {
            copy_len = sizeof(s_char_value);
            ESP_LOGW(TAG, "write value truncated to %u bytes", copy_len);
        }

        memcpy(s_char_value, param->write.value, copy_len);
        s_char_value_len = copy_len;
        esp_ble_gatts_set_attr_value(s_handle_table[IDX_CHAR_VAL], s_char_value_len, s_char_value);

        ESP_LOGI(TAG, "GATT write, conn_id=%u, len=%u", param->write.conn_id, copy_len);
        ESP_LOG_BUFFER_HEX(TAG, s_char_value, s_char_value_len);

        if (s_is_connected && s_notify_enabled && s_char_value_len > 0) {
            esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id,
                                                        s_handle_table[IDX_CHAR_VAL],
                                                        s_char_value_len, s_char_value, false);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "failed to send notification: %s", esp_err_to_name(ret));
            }
        }
    } else {
        status = ESP_GATT_INVALID_HANDLE;
    }

    if (param->write.need_rsp) {
        esp_err_t ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                                    param->write.trans_id, status, NULL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "failed to send write response: %s", esp_err_to_name(ret));
        }
    }
}

static void ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "advertising data configured, status %d", param->adv_data_raw_cmpl.status);
        s_adv_config_done &= ~ADV_CONFIG_FLAG;
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "scan response data configured, status %d", param->scan_rsp_data_raw_cmpl.status);
        s_adv_config_done &= ~SCAN_RSP_CONFIG_FLAG;
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "BLE advertising started as %s", DEVICE_NAME);
        } else {
            ESP_LOGE(TAG, "BLE advertising start failed, status %d", param->adv_start_cmpl.status);
        }
        return;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "BLE security request from " MACSTR, MAC2STR(param->ble_security.ble_req.bd_addr));
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        return;
    case ESP_GAP_BLE_NC_REQ_EVT:
        ESP_LOGI(TAG, "BLE numeric comparison request, passkey %" PRIu32,
                 param->ble_security.key_notif.passkey);
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        return;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        ESP_LOGW(TAG, "BLE passkey requested, but this example uses Just Works pairing");
        return;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        ESP_LOGI(TAG, "BLE passkey notify: %06" PRIu32, param->ble_security.key_notif.passkey);
        return;
    case ESP_GAP_BLE_KEY_EVT:
        ESP_LOGI(TAG, "BLE key exchanged, key_type=%u", param->ble_security.ble_key.key_type);
        return;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "BLE pairing complete, auth_mode=%u, peer=" MACSTR,
                     param->ble_security.auth_cmpl.auth_mode,
                     MAC2STR(param->ble_security.auth_cmpl.bd_addr));
        } else {
            ESP_LOGW(TAG, "BLE pairing failed, reason=0x%x, peer=" MACSTR,
                     param->ble_security.auth_cmpl.fail_reason,
                     MAC2STR(param->ble_security.auth_cmpl.bd_addr));
        }
        return;
    default:
        return;
    }

    if (s_adv_config_done == 0) {
        esp_err_t ret = esp_ble_gap_start_advertising(&s_adv_params);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "failed to start advertising: %s", esp_err_to_name(ret));
        }
    }
}

static void ble_gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                         esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        if (param->reg.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "GATTS app register failed, status=%d", param->reg.status);
            return;
        }

        ESP_LOGI(TAG, "GATTS app registered, app_id=%u", param->reg.app_id);
        esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if, GATTS_IDX_NB, SVC_INST_ID);
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "create attribute table failed, status=0x%x", param->add_attr_tab.status);
        } else if (param->add_attr_tab.num_handle != GATTS_IDX_NB) {
            ESP_LOGE(TAG, "attribute table handle count mismatch: got %u, expected %u",
                     param->add_attr_tab.num_handle, GATTS_IDX_NB);
        } else {
            memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(s_handle_table));
            esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
            ESP_LOGI(TAG, "GATT service 0x%04X started, char 0x%04X", s_service_uuid, s_char_uuid);
        }
        break;
    case ESP_GATTS_READ_EVT:
        handle_gatts_read(gatts_if, param);
        break;
    case ESP_GATTS_WRITE_EVT:
        handle_gatts_write(gatts_if, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "GATT MTU updated: %u", param->mtu.mtu);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "GATT service start, status=%d, handle=%u",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT: {
        s_is_connected = true;
        s_conn_id = param->connect.conn_id;
        ESP_LOGI(TAG, "GATT connected, conn_id=%u, remote=" MACSTR,
                 s_conn_id, MAC2STR(param->connect.remote_bda));

        esp_ble_conn_update_params_t conn_params = { 0 };
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = 0x20;
        conn_params.min_int = 0x10;
        conn_params.timeout = 400;
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "GATT disconnected, reason=0x%x", param->disconnect.reason);
        s_is_connected = false;
        s_notify_enabled = false;
        s_ccc_value[0] = 0;
        s_ccc_value[1] = 0;
        esp_ble_gap_start_advertising(&s_adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "GATT notification confirmed, status=%d, handle=%u",
                 param->conf.status, param->conf.handle);
        break;
    default:
        break;
    }
}

static esp_err_t start_bluedroid_gatts(void)
{
    hosted_hci_bluedroid_open();

    esp_bluedroid_hci_driver_operations_t operations = {
        .send = hosted_hci_bluedroid_send,
        .check_send_available = hosted_hci_bluedroid_check_send_available,
        .register_host_callback = hosted_hci_bluedroid_register_host_callback,
    };
    ESP_RETURN_ON_ERROR(esp_bluedroid_attach_hci_driver(&operations), TAG, "failed to attach Hosted HCI driver");
    ESP_RETURN_ON_ERROR(esp_bluedroid_init(), TAG, "failed to initialize Bluedroid");
    ESP_RETURN_ON_ERROR(esp_bluedroid_enable(), TAG, "failed to enable Bluedroid");
    ESP_RETURN_ON_ERROR(esp_ble_gap_register_callback(ble_gap_cb), TAG, "failed to register BLE GAP callback");
    ESP_RETURN_ON_ERROR(esp_ble_gatts_register_callback(ble_gatts_cb), TAG, "failed to register BLE GATTS callback");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_device_name(DEVICE_NAME), TAG, "failed to set BLE device name");
    ESP_RETURN_ON_ERROR(configure_ble_security(), TAG, "failed to configure BLE security");

    esp_bd_addr_t local_addr;
    uint8_t local_addr_type = 0;
    ESP_RETURN_ON_ERROR(esp_ble_gap_get_local_used_addr(local_addr, &local_addr_type),
                        TAG, "failed to read local BLE address");
    ESP_LOGI(TAG, "local BLE address: " MACSTR ", type: %u", MAC2STR(local_addr), local_addr_type);

    s_scan_rsp_raw_data[2] = local_addr[5];
    s_scan_rsp_raw_data[3] = local_addr[4];
    s_scan_rsp_raw_data[4] = local_addr[3];
    s_scan_rsp_raw_data[5] = local_addr[2];
    s_scan_rsp_raw_data[6] = local_addr[1];
    s_scan_rsp_raw_data[7] = local_addr[0];

    s_adv_config_done = ADV_CONFIG_FLAG | SCAN_RSP_CONFIG_FLAG;
    ESP_RETURN_ON_ERROR(esp_ble_gap_config_adv_data_raw(s_adv_raw_data, sizeof(s_adv_raw_data)),
                        TAG, "failed to configure advertising data");
    ESP_RETURN_ON_ERROR(esp_ble_gap_config_scan_rsp_data_raw(s_scan_rsp_raw_data, sizeof(s_scan_rsp_raw_data)),
                        TAG, "failed to configure scan response data");
    ESP_RETURN_ON_ERROR(esp_ble_gatts_app_register(GATTS_APP_ID), TAG, "failed to register GATTS app");

    esp_err_t mtu_ret = esp_ble_gatt_set_local_mtu(247);
    if (mtu_ret != ESP_OK) {
        ESP_LOGW(TAG, "failed to set local GATT MTU: %s", esp_err_to_name(mtu_ret));
    }

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());

    ESP_LOGI(TAG, "connecting to ESP-Hosted co-processor");
    ESP_ERROR_CHECK(esp_hosted_connect_to_slave());

    log_coprocessor_info();

    ESP_ERROR_CHECK(maybe_update_bt_mac());

    ESP_LOGI(TAG, "initializing Hosted BT controller");
    ESP_ERROR_CHECK(esp_hosted_bt_controller_init());
    ESP_ERROR_CHECK(esp_hosted_bt_controller_enable());

    ESP_ERROR_CHECK(start_bluedroid_gatts());
}
