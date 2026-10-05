#include "mtg_state.h"
#include <string.h>

void mtg_state_init(mtg_state_t *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->screen = MTG_SCREEN_BROWSER;
    state->view = MTG_VIEW_FLAVOR;
    state->net_state = MTG_NET_DISCONNECTED;
    state->battery_soc = -1;
    state->card_index = 0;

    const mtg_card_t *first = mtg_cards_get_random();
    if (first) {
        mtg_state_set_card(state, first, false);
    }
}

mtg_action_t mtg_state_handle_key(mtg_state_t *state, mtg_key_t key)
{
    if (!state) return MTG_ACTION_NONE;

    if (state->screen == MTG_SCREEN_PROVISION) {
        /* 在配网页面：任何 OK 键（单击或长按）均可退出配网，返回主界面 */
        if (key == MTG_KEY_OK_CLICK || key == MTG_KEY_OK_LONG) {
            state->screen = MTG_SCREEN_BROWSER;
            return MTG_ACTION_STOP_PROVISION;
        }
        return MTG_ACTION_NONE;
    }

    /* 主浏览页面 MTG_SCREEN_BROWSER */
    switch (key) {
    case MTG_KEY_UP:
        if (state->is_fetching) return MTG_ACTION_NONE;
        /* 向上切换视图模式：循环倒退 (效果 -> 牌面 -> 风味 -> 效果) */
        if (state->view == 0) {
            state->view = MTG_VIEW_COUNT - 1;
        } else {
            state->view = (mtg_view_t)(state->view - 1);
        }
        return MTG_ACTION_REDRAW;

    case MTG_KEY_DOWN:
        if (state->is_fetching) return MTG_ACTION_NONE;
        /* 向下切换视图模式：循环前进 (风味 -> 牌面 -> 效果 -> 风味) */
        state->view = (mtg_view_t)((state->view + 1) % MTG_VIEW_COUNT);
        return MTG_ACTION_REDRAW;

    case MTG_KEY_OK_CLICK:
        /* 正在抽牌中：静默拦截所有重复点击，杜绝网络延迟导致的请求积压与卡牌突兀闪切 */
        if (state->is_fetching) {
            return MTG_ACTION_NONE;
        }

        /* 在牌面/牌图页且处于未就绪状态（失败或尚未下载）时，单击 OK 仅重试当前牌图，不切牌 */
        if (state->view == MTG_VIEW_FACE &&
            state->net_state == MTG_NET_CONNECTED &&
            state->img_state != MTG_CARD_IMG_READY &&
            state->img_state != MTG_CARD_IMG_LOADING) {
            state->img_state = MTG_CARD_IMG_LOADING;
            return MTG_ACTION_RETRY_IMAGE;
        }
        /* 其余情况（在风味页/效果页，或牌图已就绪）：单击 OK 抽取下一张万智牌 */
        state->is_fetching = true;
        return MTG_ACTION_FETCH_CARD;

    case MTG_KEY_OK_LONG:
        /* 长按 OK：进入配网页面 */
        state->is_fetching = false;
        state->screen = MTG_SCREEN_PROVISION;
        return MTG_ACTION_START_PROVISION;

    default:
        return MTG_ACTION_NONE;
    }
}

void mtg_state_set_card(mtg_state_t *state, const mtg_card_t *card, bool is_online)
{
    if (!state || !card) return;
    state->current_card = *card;
    state->is_online_card = is_online;
    state->view = MTG_VIEW_FLAVOR;
    state->is_fetching = false;
    state->img_state = MTG_CARD_IMG_NONE;
    state->img_data = NULL;
    state->img_size = 0;
    state->img_w = 0;
    state->img_h = 0;
}

void mtg_state_set_image(mtg_state_t *state, mtg_card_img_state_t img_state,
                         const uint8_t *data, size_t size, uint16_t w, uint16_t h)
{
    if (!state) return;
    state->img_state = img_state;
    state->img_data = data;
    state->img_size = size;
    state->img_w = w;
    state->img_h = h;
}

void mtg_state_set_net_state(mtg_state_t *state, mtg_net_state_t net_state)
{
    if (!state) return;
    state->net_state = net_state;
}

void mtg_state_set_net_info(mtg_state_t *state, const char *ssid, const char *ip)
{
    if (!state) return;
    if (ssid) {
        strncpy(state->wifi_ssid, ssid, sizeof(state->wifi_ssid) - 1);
        state->wifi_ssid[sizeof(state->wifi_ssid) - 1] = '\0';
    }
    if (ip) {
        strncpy(state->ip_addr, ip, sizeof(state->ip_addr) - 1);
        state->ip_addr[sizeof(state->ip_addr) - 1] = '\0';
    }
}

void mtg_state_set_battery(mtg_state_t *state, int soc)
{
    if (!state) return;
    state->battery_soc = soc;
}

void mtg_state_set_fetching(mtg_state_t *state, bool fetching)
{
    if (!state) return;
    state->is_fetching = fetching;
}
