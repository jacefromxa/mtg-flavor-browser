#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 准备 NVS 分区（不擦除用户数据） */
esp_err_t mtg_radio_nvs_prepare(void);

/* 准备 TCP/IP 协议栈与默认系统事件循环 */
esp_err_t mtg_radio_network_prepare(void);

#ifdef __cplusplus
}
#endif
