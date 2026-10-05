#pragma once

#include "mtg_cards.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化获取模块 */
void mtg_fetch_init(void);

/* 请求获取一张随机卡牌（若在线则拉取线上全量牌库数据，离线或失败自动从本地 1000 库回退）
 * @param out_card 输出填充的卡牌结构
 * @param is_online 输出该卡牌是否成功来自网络
 * @return 是否成功获取（离线回退也视为成功）
 */
bool mtg_fetch_random_card(mtg_card_t *out_card, bool *is_online);

/* 拉取卡牌图片（JPEG 格式）
 * @param card 目标卡牌
 * @param out_buf 输出接收缓冲区
 * @param buf_size 缓冲区最大容量
 * @param out_len 实际读取的字节数
 * @param out_w 输出解析得到的图片宽度
 * @param out_h 输出解析得到的图片高度
 * @return 是否成功拉取并验证为有效 JPEG
 */
bool mtg_fetch_card_image(const mtg_card_t *card, uint8_t *out_buf, size_t buf_size,
                          size_t *out_len, uint16_t *out_w, uint16_t *out_h);

/* Wi-Fi 联网成功时触发离线库刷新与在线数据同步 */
void mtg_fetch_on_wifi_connected(void);

#ifdef __cplusplus
}
#endif
