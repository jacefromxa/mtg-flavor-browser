#include "mtg_blufi.h"
#include "mtg_blufi_security.h"
#include "mtg_radio.h"

#include "esp_blufi.h"
#include "esp_blufi_api.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "lwip/ip4_addr.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include <string.h>

static const char *TAG = "mtg_blufi";
static const char *DEVICE_NAME = "BLUFI_FoloPassport";

#define BLUFI_AP_LIST_COUNT 16

static mtg_blufi_status_cb_t s_status_cb = NULL;
static esp_netif_t *s_sta_netif = NULL;
static esp_event_handler_instance_t s_wifi_handler = NULL;
static esp_event_handler_instance_t s_ip_handler = NULL;
static wifi_config_t s_sta_config;
static char s_current_ip[16] = {0};
static char s_current_ssid[33] = {0};

static bool s_wifi_initialized = false;
static bool s_wifi_started = false;
static bool s_wifi_connected = false;
static bool s_wifi_connecting = false;
static bool s_ble_connected = false;
static bool s_host_initialized = false;
static bool s_host_running = false;
static bool s_gatt_initialized = false;
static bool s_btc_initialized = false;
static bool s_profile_initialized = false;
static SemaphoreHandle_t s_host_stopped = NULL;

static void notify_status(mtg_blufi_status_t st)
{
    if (s_status_cb) {
        s_status_cb(st, s_current_ssid, s_current_ip);
    }
}

static void send_wifi_report(esp_blufi_sta_conn_state_t state)
{
    wifi_mode_t mode = WIFI_MODE_STA;
    esp_wifi_get_mode(&mode);
    esp_blufi_extra_info_t info = { 0 };
    size_t ssid_len = strnlen((const char *)s_sta_config.sta.ssid, sizeof(s_sta_config.sta.ssid));
    if (ssid_len > 0) {
        info.sta_ssid = s_sta_config.sta.ssid;
        info.sta_ssid_len = ssid_len;
    }
    esp_blufi_send_wifi_conn_report(mode, state, 0, &info);
}

static void send_wifi_list(void)
{
    uint16_t count = BLUFI_AP_LIST_COUNT;
    wifi_ap_record_t records[BLUFI_AP_LIST_COUNT] = { 0 };
    esp_blufi_ap_record_t list[BLUFI_AP_LIST_COUNT] = { 0 };
    esp_err_t err = esp_wifi_scan_get_ap_records(&count, records);
    if (err != ESP_OK) {
        esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        return;
    }
    for (uint16_t i = 0; i < count; i++) {
        list[i].rssi = records[i].rssi;
        memcpy(list[i].ssid, records[i].ssid, sizeof(list[i].ssid));
    }
    if (s_ble_connected) {
        esp_blufi_send_wifi_list(count, list);
    }
}

static void request_wifi_connect(void)
{
    s_wifi_connecting = true;
    s_wifi_connected = false;
    notify_status(MTG_BLUFI_CONNECTING);

    esp_wifi_disconnect();
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_connect 启动失败: %s", esp_err_to_name(err));
        s_wifi_connecting = false;
        send_wifi_report(ESP_BLUFI_STA_CONN_FAIL);
        notify_status(MTG_BLUFI_DISCONNECTED);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;
    if (id == WIFI_EVENT_STA_START) {
        esp_wifi_get_config(WIFI_IF_STA, &s_sta_config);
        if (s_sta_config.sta.ssid[0] != '\0') {
            ESP_LOGI(TAG, "已读取保存的 Wi-Fi 凭据 (%s)，自动连接...", (char *)s_sta_config.sta.ssid);
            strncpy(s_current_ssid, (char *)s_sta_config.sta.ssid, sizeof(s_current_ssid) - 1);
            s_current_ssid[sizeof(s_current_ssid) - 1] = '\0';
            request_wifi_connect();
        }
    } else if (id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi AP 已关联，等待分配 IP");
        notify_status(MTG_BLUFI_CONNECTING);
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi 连接已断开或连接失败");
        s_wifi_connected = false;
        s_wifi_connecting = false;
        s_current_ip[0] = '\0';
        send_wifi_report(ESP_BLUFI_STA_CONN_FAIL);
        notify_status(MTG_BLUFI_DISCONNECTED);
    } else if (id == WIFI_EVENT_SCAN_DONE) {
        send_wifi_list();
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
    esp_ip4addr_ntoa(&event->ip_info.ip, s_current_ip, sizeof(s_current_ip));
    s_wifi_connected = true;
    s_wifi_connecting = false;
    ESP_LOGI(TAG, "成功分配到 IP 地址: %s", s_current_ip);
    send_wifi_report(ESP_BLUFI_STA_CONN_SUCCESS);
    notify_status(MTG_BLUFI_CONNECTED);
}

static void blufi_reset(int reason)
{
    ESP_LOGE(TAG, "NimBLE 复位，原因: %d", reason);
}

static void blufi_sync(void)
{
    int rc = esp_blufi_profile_init();
    if (rc == 0) {
        s_profile_initialized = true;
        ESP_LOGI(TAG, "BLUFI profile 初始化成功");
    } else {
        ESP_LOGE(TAG, "esp_blufi_profile_init 失败: %d", rc);
    }
}

static void host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    if (s_host_stopped) xSemaphoreGive(s_host_stopped);
    nimble_port_freertos_deinit();
}

static void blufi_event(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        ESP_LOGI(TAG, "BLUFI 初始化完成，开启广播: %s", DEVICE_NAME);
        esp_blufi_adv_start_with_name(DEVICE_NAME);
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        ESP_LOGI(TAG, "BLUFI 逆初始化完成");
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        s_ble_connected = true;
        ESP_LOGI(TAG, "手机 BLE 已连接，开始安全协商");
        esp_blufi_adv_stop();
        mtg_blufi_security_init();
        notify_status(MTG_BLUFI_BLE_CONNECTED);
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        s_ble_connected = false;
        ESP_LOGI(TAG, "手机 BLE 已断开");
        mtg_blufi_security_deinit();
        notify_status(MTG_BLUFI_BLE_DISCONNECTED);
        if (s_host_running) {
            esp_blufi_adv_start_with_name(DEVICE_NAME);
        }
        break;
    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        esp_wifi_set_mode(WIFI_MODE_STA);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        memcpy(s_sta_config.sta.bssid, param->sta_bssid.bssid, 6);
        s_sta_config.sta.bssid_set = true;
        esp_wifi_set_config(WIFI_IF_STA, &s_sta_config);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
        if (param->sta_ssid.ssid_len >= sizeof(s_sta_config.sta.ssid)) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            break;
        }
        memset(s_sta_config.sta.ssid, 0, sizeof(s_sta_config.sta.ssid));
        memset(s_sta_config.sta.password, 0, sizeof(s_sta_config.sta.password));
        memset(s_sta_config.sta.bssid, 0, sizeof(s_sta_config.sta.bssid));
        s_sta_config.sta.bssid_set = false;
        s_sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        memcpy(s_sta_config.sta.ssid, param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        strncpy(s_current_ssid, (char *)s_sta_config.sta.ssid, sizeof(s_current_ssid) - 1);
        esp_wifi_set_config(WIFI_IF_STA, &s_sta_config);
        ESP_LOGI(TAG, "正确接收到配网 SSID: %s (长度: %d)", s_current_ssid, param->sta_ssid.ssid_len);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        if (param->sta_passwd.passwd_len >= sizeof(s_sta_config.sta.password)) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            break;
        }
        memset(s_sta_config.sta.password, 0, sizeof(s_sta_config.sta.password));
        memcpy(s_sta_config.sta.password, param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        esp_wifi_set_config(WIFI_IF_STA, &s_sta_config);
        ESP_LOGI(TAG, "正确接收到配网密码 (长度: %d)", param->sta_passwd.passwd_len);
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        ESP_LOGI(TAG, "小程序请求连接 AP...");
        request_wifi_connect();
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        esp_wifi_disconnect();
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
        if (s_wifi_connected) {
            send_wifi_report(ESP_BLUFI_STA_CONN_SUCCESS);
        } else if (s_wifi_connecting) {
            send_wifi_report(ESP_BLUFI_STA_CONNECTING);
        } else {
            send_wifi_report(ESP_BLUFI_STA_CONN_FAIL);
        }
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST: {
        wifi_scan_config_t scan_config = { 0 };
        esp_wifi_scan_start(&scan_config, false);
        break;
    }
    default:
        break;
    }
}

static esp_blufi_callbacks_t s_callbacks = {
    .event_cb = blufi_event,
    .negotiate_data_handler = mtg_blufi_negotiate,
    .encrypt_func = mtg_blufi_encrypt,
    .decrypt_func = mtg_blufi_decrypt,
    .checksum_func = mtg_blufi_checksum,
};

static esp_err_t ensure_host_started(void)
{
    if (s_host_initialized) return ESP_OK;

    esp_err_t err = esp_blufi_register_callbacks(&s_callbacks);
    if (err != ESP_OK) return err;

    err = nimble_port_init();
    if (err != ESP_OK) return err;

    s_host_initialized = true;
    s_host_stopped = xSemaphoreCreateBinary();
    if (!s_host_stopped) return ESP_ERR_NO_MEM;

    ble_hs_cfg.reset_cb = blufi_reset;
    ble_hs_cfg.sync_cb = blufi_sync;
    ble_hs_cfg.gatts_register_cb = esp_blufi_gatt_svr_register_cb;

    int rc = esp_blufi_gatt_svr_init();
    if (rc != 0) return ESP_FAIL;
    s_gatt_initialized = true;

    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) return ESP_FAIL;

    esp_blufi_btc_init();
    s_btc_initialized = true;

    err = esp_nimble_enable(host_task);
    if (err == ESP_OK) {
        s_host_running = true;
    }
    return err;
}

esp_err_t mtg_blufi_init(mtg_blufi_status_cb_t cb)
{
    s_status_cb = cb;

    esp_err_t err = mtg_radio_nvs_prepare();
    if (err != ESP_OK) return err;
    err = mtg_radio_network_prepare();
    if (err != ESP_OK) return err;

    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (!s_sta_netif) return ESP_ERR_NO_MEM;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;
    s_wifi_initialized = true;

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              wifi_event_handler, NULL, &s_wifi_handler);
    if (err != ESP_OK) return err;

    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              ip_event_handler, NULL, &s_ip_handler);
    if (err != ESP_OK) return err;

    err = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    if (err == ESP_OK) err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) err = esp_wifi_start();
    if (err == ESP_OK) s_wifi_started = true;

    return err;
}

esp_err_t mtg_blufi_start_adv(void)
{
    esp_err_t err = ensure_host_started();
    if (err != ESP_OK) return err;

    if (s_profile_initialized) {
        esp_blufi_adv_start_with_name(DEVICE_NAME);
    }
    return ESP_OK;
}

esp_err_t mtg_blufi_stop_adv(void)
{
    if (s_profile_initialized) {
        esp_blufi_adv_stop();
    }
    return ESP_OK;
}

bool mtg_blufi_is_connected(void)
{
    return s_wifi_connected;
}

void mtg_blufi_get_info(char *ssid_out, size_t ssid_len, char *ip_out, size_t ip_len)
{
    if (ssid_out && ssid_len > 0) {
        strncpy(ssid_out, s_current_ssid, ssid_len - 1);
        ssid_out[ssid_len - 1] = '\0';
    }
    if (ip_out && ip_len > 0) {
        strncpy(ip_out, s_current_ip, ip_len - 1);
        ip_out[ip_len - 1] = '\0';
    }
}
