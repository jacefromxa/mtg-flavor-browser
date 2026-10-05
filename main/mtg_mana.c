#include "mtg_mana.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static int encode_utf8(uint32_t cp, char *out)
{
    if (cp <= 0x7F) {
        out[0] = (char)cp;
        out[1] = '\0';
        return 1;
    } else if (cp <= 0x7FF) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        out[2] = '\0';
        return 2;
    } else if (cp <= 0xFFFF) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        out[3] = '\0';
        return 3;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        out[4] = '\0';
        return 4;
    }
}

static uint32_t resolve_symbol_glyph(const char *sym, size_t len, uint32_t *out_bg, uint32_t *out_fg)
{
    uint32_t glyph = 0;
    uint32_t bg = 0xcbc2bf; /* 默认通用法术力底色：浅石灰 */
    uint32_t fg = 0x1a1a1a; /* 默认黑色文字 */

    if (len == 1) {
        char c = toupper((unsigned char)sym[0]);
        switch (c) {
        case 'W':
            glyph = MANA_GLYPH_W;
            bg = 0xfffbd5; /* 象牙暖白 */
            fg = 0x1a1a1a;
            break;
        case 'U':
            glyph = MANA_GLYPH_U;
            bg = 0x0e68ab; /* 蔚蓝 */
            fg = 0xffffff;
            break;
        case 'B':
            glyph = MANA_GLYPH_B;
            bg = 0x211c1d; /* 炭黑 */
            fg = 0xffffff;
            break;
        case 'R':
            glyph = MANA_GLYPH_R;
            bg = 0xd3202a; /* 烈红 */
            fg = 0xffffff;
            break;
        case 'G':
            glyph = MANA_GLYPH_G;
            bg = 0x00733e; /* 翠绿 */
            fg = 0xffffff;
            break;
        case 'C':
            glyph = MANA_GLYPH_C;
            bg = 0xccc2c0; /* 无色石灰 */
            fg = 0x1a1a1a;
            break;
        case 'T':
            glyph = MANA_GLYPH_TAP;
            bg = 0x73706e;
            fg = 0xffffff;
            break;
        case 'X':
            glyph = MANA_GLYPH_X;
            bg = 0xcbc2bf;
            fg = 0x1a1a1a;
            break;
        case '0': glyph = MANA_GLYPH_0; break;
        case '1': glyph = MANA_GLYPH_1; break;
        case '2': glyph = MANA_GLYPH_2; break;
        case '3': glyph = MANA_GLYPH_3; break;
        case '4': glyph = MANA_GLYPH_4; break;
        case '5': glyph = MANA_GLYPH_5; break;
        case '6': glyph = MANA_GLYPH_6; break;
        case '7': glyph = MANA_GLYPH_7; break;
        case '8': glyph = MANA_GLYPH_8; break;
        case '9': glyph = MANA_GLYPH_9; break;
        default: break;
        }
    } else if (len == 2) {
        if (sym[0] == '1') {
            if (sym[1] >= '0' && sym[1] <= '5') {
                glyph = MANA_GLYPH_10 + (sym[1] - '0');
            } else if (sym[1] == '6') {
                glyph = MANA_GLYPH_16;
            }
        } else if (sym[0] == '2' && sym[1] == '0') {
            glyph = MANA_GLYPH_20;
        }
    } else if (len == 3) {
        if (strncasecmp(sym, "tap", 3) == 0) {
            glyph = MANA_GLYPH_TAP;
            bg = 0x73706e;
            fg = 0xffffff;
        }
    }

    if (out_bg) *out_bg = bg;
    if (out_fg) *out_fg = fg;
    return glyph;
}

size_t mtg_mana_parse_badges(const char *cost_str, mtg_mana_badge_t *out_badges, size_t max_badges)
{
    if (!cost_str || !out_badges || max_badges == 0) return 0;

    size_t count = 0;
    size_t len = strlen(cost_str);
    size_t i = 0;

    while (i < len && count < max_badges) {
        if (cost_str[i] == '{') {
            size_t j = i + 1;
            while (j < len && cost_str[j] != '}' && (j - i) < 10) {
                j++;
            }
            if (j < len && cost_str[j] == '}') {
                size_t sym_len = j - (i + 1);
                uint32_t bg = 0, fg = 0;
                uint32_t glyph = resolve_symbol_glyph(&cost_str[i + 1], sym_len, &bg, &fg);
                if (glyph != 0) {
                    out_badges[count].glyph = glyph;
                    out_badges[count].bg_color = bg;
                    out_badges[count].text_color = fg;
                    encode_utf8(glyph, out_badges[count].label_utf8);
                    count++;
                }
                i = j + 1;
                continue;
            }
        }
        i++;
    }
    return count;
}

void mtg_mana_format_text(const char *in, char *out, size_t out_size)
{
    if (!out || out_size == 0) return;
    if (!in) {
        out[0] = '\0';
        return;
    }

    size_t out_idx = 0;
    size_t in_len = strlen(in);
    size_t i = 0;

    while (i < in_len && out_idx + 1 < out_size) {
        if (in[i] == '{') {
            size_t j = i + 1;
            while (j < in_len && in[j] != '}' && (j - i) < 10) {
                j++;
            }
            if (j < in_len && in[j] == '}') {
                uint32_t glyph = resolve_symbol_glyph(&in[i + 1], j - (i + 1), NULL, NULL);
                if (glyph != 0) {
                    char utf8_buf[8];
                    int utf8_len = encode_utf8(glyph, utf8_buf);
                    if (out_idx + utf8_len < out_size) {
                        for (int k = 0; k < utf8_len; k++) {
                            out[out_idx++] = utf8_buf[k];
                        }
                        i = j + 1;
                        continue;
                    }
                }
            }
        }
        out[out_idx++] = in[i++];
    }
    out[out_idx] = '\0';
}
