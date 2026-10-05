#include "mtg_mana.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char buf[128];

    /* 基础颜色法术力 */
    mtg_mana_format_text("{R}", buf, sizeof(buf));
    assert(strlen(buf) == 3);
    assert((unsigned char)buf[0] == 0xEE && (unsigned char)buf[1] == 0x98 && (unsigned char)buf[2] == 0x83);

    /* 复合费用 */
    mtg_mana_format_text("{2}{W}{U}", buf, sizeof(buf));
    assert(strlen(buf) == 9);

    /* 规则文本中内联法术力 */
    mtg_mana_format_text("闪电击对任意目标造成3点伤害。{T}：加{C}{C}。", buf, sizeof(buf));
    assert(strstr(buf, "{T}") == NULL);
    assert(strstr(buf, "{C}") == NULL);

    /* 带底色徽章解析测试 */
    mtg_mana_badge_t badges[8];
    size_t count = mtg_mana_parse_badges("{2}{W}{U}{B}{R}{G}", badges, 8);
    assert(count == 6);
    assert(badges[0].bg_color == 0xcbc2bf); /* 2: 灰色底 */
    assert(badges[0].text_color == 0x1a1a1a); /* 深色标 */
    assert(badges[1].bg_color == 0xfffbd5); /* W: 暖白底 */
    assert(badges[1].text_color == 0x1a1a1a); /* 深色标 */
    assert(badges[2].bg_color == 0x0e68ab); /* U: 蔚蓝底 */
    assert(badges[2].text_color == 0xffffff); /* 白标 */
    assert(badges[3].bg_color == 0x211c1d); /* B: 炭黑底 */
    assert(badges[3].text_color == 0xffffff); /* 白标 */
    assert(badges[4].bg_color == 0xd3202a); /* R: 烈红底 */
    assert(badges[4].text_color == 0xffffff); /* 白标 */
    assert(badges[5].bg_color == 0x00733e); /* G: 翠绿底 */
    assert(badges[5].text_color == 0xffffff); /* 白标 */

    printf("ALL MTG MANA SYMBOL & BADGE TESTS PASSED!\n");
    return 0;
}
