#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MTG_BLUFI_DISCONNECTED = 0, /* Wi-Fi 断开 */
    MTG_BLUFI_CONNECTING,       /* Wi-Fi 正在连接 */
    MTG_BLUFI_CONNECTED,        /* Wi-Fi 已连接并分配到 IP */
    MTG_BLUFI_BLE_CONNECTED,    /* 蓝牙手机已连接 */
    MTG_BLUFI_BLE_DISCONNECTED, /* 蓝牙手机已断开 */
} mtg_blufi_status_t;

typedef void (*mtg_blufi_status_cb_t)(mtg_blufi_status_t status, const char *ssid, const char *ip);

/* 初始化 Wi-Fi 与 BLUFI 模块，并尝试自动连接已存储的 Wi-Fi 凭据 */
esp_err_t mtg_blufi_init(mtg_blufi_status_cb_t cb);

/* 开启蓝牙 BLUFI 广播（进入配网模式） */
esp_err_t mtg_blufi_start_adv(void);

/* 停止蓝牙 BLUFI 广播（退出配网模式） */
esp_err_t mtg_blufi_stop_adv(void);

/* 查询 Wi-Fi 是否已连接并获得 IP */
bool mtg_blufi_is_connected(void);

/* 获取当前连接的 SSID 与 IP 字符串 */
void mtg_blufi_get_info(char *ssid_out, size_t ssid_len, char *ip_out, size_t ip_len);

#ifdef __cplusplus
}
#endif
