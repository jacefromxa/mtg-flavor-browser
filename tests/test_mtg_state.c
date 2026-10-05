#include "mtg_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_cards_init_and_query(void)
{
    mtg_cards_init(42);
    size_t count = mtg_cards_count();
    assert(count > 0);

    const mtg_card_t *c0 = mtg_cards_get_by_index(0);
    assert(c0 != NULL);
    assert(strcmp(c0->name_zh, "闪电击") == 0);
    assert(strlen(c0->flavor_text) > 0);

    const mtg_card_t *rnd = mtg_cards_get_random();
    assert(rnd != NULL);
    assert(strlen(rnd->name_zh) > 0);
    printf("test_cards_init_and_query: PASS (count=%zu)\n", count);
}

static void test_state_navigation(void)
{
    mtg_state_t s;
    mtg_state_init(&s);

    assert(s.screen == MTG_SCREEN_BROWSER);
    assert(s.view == MTG_VIEW_FLAVOR);
    assert(strlen(s.current_card.name_zh) > 0);

    /* DOWN 键向下切换：FLAVOR -> FACE -> ORACLE -> FLAVOR */
    mtg_action_t a1 = mtg_state_handle_key(&s, MTG_KEY_DOWN);
    assert(a1 == MTG_ACTION_REDRAW);
    assert(s.view == MTG_VIEW_FACE);

    mtg_action_t a2 = mtg_state_handle_key(&s, MTG_KEY_DOWN);
    assert(a2 == MTG_ACTION_REDRAW);
    assert(s.view == MTG_VIEW_ORACLE);

    mtg_action_t a3 = mtg_state_handle_key(&s, MTG_KEY_DOWN);
    assert(a3 == MTG_ACTION_REDRAW);
    assert(s.view == MTG_VIEW_FLAVOR);

    /* UP 键向上切换：FLAVOR -> ORACLE -> FACE -> FLAVOR */
    mtg_action_t a4 = mtg_state_handle_key(&s, MTG_KEY_UP);
    assert(a4 == MTG_ACTION_REDRAW);
    assert(s.view == MTG_VIEW_ORACLE);

    mtg_action_t a5 = mtg_state_handle_key(&s, MTG_KEY_UP);
    assert(a5 == MTG_ACTION_REDRAW);
    assert(s.view == MTG_VIEW_FACE);

    mtg_action_t a6 = mtg_state_handle_key(&s, MTG_KEY_UP);
    assert(a6 == MTG_ACTION_REDRAW);
    /* 测试切换新牌时强制重置回风味文字视图 */
    s.view = MTG_VIEW_ORACLE;
    const mtg_card_t *c = mtg_cards_get_by_index(1);
    mtg_state_set_card(&s, c, false);
    assert(s.view == MTG_VIEW_FLAVOR);

    printf("test_state_navigation: PASS\n");
}

static void test_state_actions(void)
{
    mtg_state_t s;
    mtg_state_init(&s);

    /* 单击 OK 键触发抽取下一张牌 */
    mtg_action_t a_fetch = mtg_state_handle_key(&s, MTG_KEY_OK_CLICK);
    assert(a_fetch == MTG_ACTION_FETCH_CARD);
    assert(s.is_fetching == true);

    /* 抽牌进行中再次单击 OK 键应被严格静默拦截 (返回 MTG_ACTION_NONE) */
    mtg_action_t a_dup = mtg_state_handle_key(&s, MTG_KEY_OK_CLICK);
    assert(a_dup == MTG_ACTION_NONE);
    assert(s.is_fetching == true);

    /* 卡牌到达后设置新卡牌，is_fetching 恢复为 false */
    const mtg_card_t *c = mtg_cards_get_by_index(0);
    mtg_state_set_card(&s, c, false);
    assert(s.is_fetching == false);

    /* 恢复后可正常再次抽牌 */
    a_fetch = mtg_state_handle_key(&s, MTG_KEY_OK_CLICK);
    assert(a_fetch == MTG_ACTION_FETCH_CARD);
    assert(s.is_fetching == true);

    /* 长按 OK 键进入配网页面 */
    mtg_action_t a_prov = mtg_state_handle_key(&s, MTG_KEY_OK_LONG);
    assert(a_prov == MTG_ACTION_START_PROVISION);
    assert(s.screen == MTG_SCREEN_PROVISION);
    assert(s.is_fetching == false);

    /* 配网页面下单击或长按 OK 键退出配网 */
    mtg_action_t a_stop = mtg_state_handle_key(&s, MTG_KEY_OK_CLICK);
    assert(a_stop == MTG_ACTION_STOP_PROVISION);
    assert(s.screen == MTG_SCREEN_BROWSER);

    /* 测试网络状态与电量设置 */
    mtg_state_set_net_state(&s, MTG_NET_CONNECTED);
    assert(s.net_state == MTG_NET_CONNECTED);

    mtg_state_set_net_info(&s, "MyHomeWiFi", "192.168.1.100");
    assert(strcmp(s.wifi_ssid, "MyHomeWiFi") == 0);
    assert(strcmp(s.ip_addr, "192.168.1.100") == 0);

    mtg_state_set_battery(&s, 88);
    assert(s.battery_soc == 88);

    printf("test_state_actions: PASS\n");
}

static void test_deck_shuffle_no_repeats(void)
{
    mtg_cards_init(12345);
    size_t count = mtg_cards_count();
    assert(count == 1000);

    /* 验证在一整轮 1000 次抽取内，每一张牌均唯一出现（0 重复率） */
    const mtg_card_t **drawn = malloc(sizeof(const mtg_card_t *) * count);
    assert(drawn != NULL);
    for (size_t i = 0; i < count; i++) {
        const mtg_card_t *c = mtg_cards_get_random();
        assert(c != NULL);
        for (size_t j = 0; j < i; j++) {
            assert(c != drawn[j]); /* 确保不重复指针 */
        }
        drawn[i] = c;
    }

    /* 抽满 1000 张后下一张应触发自动重洗且不与第 1000 张相同 */
    const mtg_card_t *next_cycle_first = mtg_cards_get_random();
    assert(next_cycle_first != NULL);
    assert(next_cycle_first != drawn[count - 1]);
    free(drawn);

    printf("test_deck_shuffle_no_repeats: PASS (all %zu cards strictly unique per cycle)\n", count);
}

int main(void)
{
    test_cards_init_and_query();
    test_state_navigation();
    test_state_actions();
    test_deck_shuffle_no_repeats();
    printf("ALL MTG STATE HOST TESTS PASSED!\n");
    return 0;
}
