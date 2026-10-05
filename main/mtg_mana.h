#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mana 符号官方 Unicode 码位 */
#define MANA_GLYPH_W    0xE600
#define MANA_GLYPH_U    0xE601
#define MANA_GLYPH_B    0xE602
#define MANA_GLYPH_R    0xE603
#define MANA_GLYPH_G    0xE604
#define MANA_GLYPH_C    0xE904
#define MANA_GLYPH_TAP  0xE61A
#define MANA_GLYPH_X    0xE615

#define MANA_GLYPH_0    0xE605
#define MANA_GLYPH_1    0xE606
#define MANA_GLYPH_2    0xE607
#define MANA_GLYPH_3    0xE608
#define MANA_GLYPH_4    0xE609
#define MANA_GLYPH_5    0xE60A
#define MANA_GLYPH_6    0xE60B
#define MANA_GLYPH_7    0xE60C
#define MANA_GLYPH_8    0xE60D
#define MANA_GLYPH_9    0xE60E
#define MANA_GLYPH_10   0xE60F
#define MANA_GLYPH_11   0xE610
#define MANA_GLYPH_12   0xE611
#define MANA_GLYPH_13   0xE612
#define MANA_GLYPH_14   0xE613
#define MANA_GLYPH_15   0xE614
#define MANA_GLYPH_16   0xE62A
#define MANA_GLYPH_20   0xE62E

/* 单个法术力徽章定义（包含官方底色与前景色） */
typedef struct {
    uint32_t glyph;       /* Unicode 码位 */
    uint32_t bg_color;    /* RGB888 官方背景底色 */
    uint32_t text_color;  /* RGB888 符号文字颜色 */
    char label_utf8[8];   /* UTF-8 编码文本 */
} mtg_mana_badge_t;

/* 从费用字符串解析最多 max_badges 个带底色的法术力徽章 */
size_t mtg_mana_parse_badges(const char *cost_str, mtg_mana_badge_t *out_badges, size_t max_badges);

/* 格式化内联文本中的法术力符号 */
void mtg_mana_format_text(const char *in, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
