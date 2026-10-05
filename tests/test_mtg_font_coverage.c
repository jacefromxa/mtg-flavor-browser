#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

extern const uint32_t mtg_font_codepoints[];
extern const size_t mtg_font_codepoint_count;

static bool has_codepoint(uint32_t cp)
{
    /* 二分查找 */
    size_t left = 0;
    size_t right = mtg_font_codepoint_count;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (mtg_font_codepoints[mid] == cp) return true;
        if (mtg_font_codepoints[mid] < cp) left = mid + 1;
        else right = mid;
    }
    return false;
}

int main(void)
{
    assert(mtg_font_codepoint_count > 0);
    printf("Verifying font codepoint coverage (%zu codepoints)...\n", mtg_font_codepoint_count);

    /* 验证基础 ASCII 均被覆盖 */
    for (uint32_t c = 0x20; c < 0x7f; c++) {
        assert(has_codepoint(c));
    }

    /* 验证核心中文关键字均被覆盖 */
    const char *test_words = "万智牌风味箴言牌面效果规则闪电击黑莲花神之愤怒反击咒语黑暗祭礼巨木吞噬怪西瓦巨龙塞拉天使杰斯时间漫步泰菲力莉莲娜茜卓妮莎配网状态连接离线成功返回热点网页手机家中";
    const uint8_t *p = (const uint8_t *)test_words;
    while (*p) {
        uint32_t cp = 0;
        if ((*p & 0x80) == 0) {
            cp = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
            cp = (*p++ & 0x1F) << 6;
            cp |= (*p++ & 0x3F);
        } else if ((*p & 0xF0) == 0xE0) {
            cp = (*p++ & 0x0F) << 12;
            cp |= (*p++ & 0x3F) << 6;
            cp |= (*p++ & 0x3F);
        } else if ((*p & 0xF8) == 0xF0) {
            cp = (*p++ & 0x07) << 18;
            cp |= (*p++ & 0x3F) << 12;
            cp |= (*p++ & 0x3F) << 6;
            cp |= (*p++ & 0x3F);
        } else {
            p++;
            continue;
        }
        if (!has_codepoint(cp)) {
            fprintf(stderr, "Missing glyph for U+%04X!\n", cp);
            assert(false);
        }
    }

    /* 验证 Mana Font 官方符号均被覆盖 */
    const uint32_t mana_symbols[] = {
        0xE600, 0xE601, 0xE602, 0xE603, 0xE604, 0xE904, 0xE61A, 0xE615,
        0xE605, 0xE606, 0xE607, 0xE608, 0xE609, 0xE60A, 0xE60B, 0xE60C
    };
    for (size_t i = 0; i < sizeof(mana_symbols) / sizeof(mana_symbols[0]); i++) {
        assert(has_codepoint(mana_symbols[i]));
    }

    printf("ALL MTG FONT GLYPH COVERAGE TESTS PASSED!\n");
    return 0;
}
