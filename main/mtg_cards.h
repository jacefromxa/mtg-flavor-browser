#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name_zh;        /* 中文牌名 */
    const char *name_en;        /* 英文原名 */
    const char *mana_cost;      /* 法术力费用字符串，如 {1}{U}{R} */
    const char *type_line;      /* 类别，如 "生物 ～ 天使" */
    const char *oracle_text;    /* 规则叙述 */
    const char *flavor_text;    /* 风味文字 (中文) */
    const char *flavor_en;      /* 风味文字 (英文) */
    const char *flavor_author;  /* 引言出处/作者 */
    const char *pt;             /* 力量/防御力，如 "4/4" */
    const char *set_code;       /* 系列缩写，如 "LEA #161" */
    const char *image_url;      /* 牌图 URL (Scryfall small jpg，若有) */
} mtg_card_t;

/* 初始化卡牌数据库与洗牌队列 */
void mtg_cards_init(uint32_t seed);

/* 重新洗牌与刷新卡牌周期 (联网时调用，确保每次联网刷新离线库) */
void mtg_cards_reshuffle(uint32_t seed);

/* 获取卡牌库总数 */
size_t mtg_cards_count(void);

/* 按索引获取卡牌 (0 ~ count-1) */
const mtg_card_t *mtg_cards_get_by_index(size_t index);

/* 随机抽取一张卡牌 (基于无放回洗牌算法，保证每个完整周期内绝不重复) */
const mtg_card_t *mtg_cards_get_random(void);

#ifdef __cplusplus
}
#endif
