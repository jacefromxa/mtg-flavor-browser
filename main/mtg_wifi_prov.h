#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MTG_PROV_DISCONNECTED = 0, /* 未连接 */
    MTG_PROV_CONNECTING,       /* 正在连接 Wi-Fi */
    MTG_PROV_CONNECTED,        /* 已连接并获取 IP */
    MTG_PROV_AP_ACTIVE,        /* 配网热点已开启 (等待用户在网页配网) */
    MTG_PROV_FAIL,             /* 连接失败 */
} mtg_wifi_prov_status_t;

typedef void (*mtg_wifi_prov_cb_t)(mtg_wifi_prov_status_t status, const char *ssid, const char *ip);

/* 初始化 Wi-Fi 栈；如果 NVS 中有保存的历史 Wi-Fi 则自动连接 */
esp_err_t mtg_wifi_prov_init(mtg_wifi_prov_cb_t cb);

/* 开启 SoftAP 热点与网页配网服务 (192.168.4.1) */
esp_err_t mtg_wifi_prov_start(void);

/* 关闭 SoftAP 热点配网服务，恢复纯 STA 模式 */
esp_err_t mtg_wifi_prov_stop(void);

/* 当前是否已连上可用 Wi-Fi */
bool mtg_wifi_prov_is_connected(void);

/* 获取当前 SSID 与 IP */
const char *mtg_wifi_prov_get_ssid(void);
const char *mtg_wifi_prov_get_ip(void);

#ifdef __cplusplus
}
#endif
