#pragma once

#include "mtg_cards.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 浏览视图类型 */
typedef enum {
    MTG_VIEW_FLAVOR = 0, /* 风味文字 / 经典箴言视图 */
    MTG_VIEW_FACE,       /* 牌面全景视图（牌名、费用、类别、攻防） */
    MTG_VIEW_ORACLE,     /* 牌效果视图（规则描述、Oracle 文本） */
    MTG_VIEW_COUNT,
} mtg_view_t;

/* 屏幕模式 */
typedef enum {
    MTG_SCREEN_BROWSER = 0, /* 主卡牌浏览界面 */
    MTG_SCREEN_PROVISION,   /* BLUFI 配网界面 */
} mtg_screen_t;

/* 网络连接状态 */
typedef enum {
    MTG_NET_DISCONNECTED = 0, /* 离线 / 未连接 */
    MTG_NET_CONNECTING,       /* Wi-Fi 正在连接中 */
    MTG_NET_CONNECTED,        /* Wi-Fi 已连接并获得 IP */
    MTG_NET_PROV_ACTIVE,      /* Wi-Fi 热点网页配网模式开启中 (192.168.4.1) */
    MTG_NET_BLUFI_ACTIVE = MTG_NET_PROV_ACTIVE, /* 别名兼容 */
} mtg_net_state_t;

/* 输入按键事件 */
typedef enum {
    MTG_KEY_UP = 0,
    MTG_KEY_DOWN,
    MTG_KEY_OK_CLICK,
    MTG_KEY_OK_LONG,
} mtg_key_t;

/* 状态转移结果动作 */
typedef enum {
    MTG_ACTION_NONE = 0,
    MTG_ACTION_REDRAW,         /* 需要重绘 UI */
    MTG_ACTION_FETCH_CARD,     /* 需要抽取下一张卡牌（优先网络，失败回退） */
    MTG_ACTION_RETRY_IMAGE,    /* 在牌图页仅重试拉取当前牌图 */
    MTG_ACTION_START_PROVISION,/* 启动 BLUFI 广播并显示配网页面 */
    MTG_ACTION_STOP_PROVISION, /* 停止 BLUFI 并返回卡牌浏览页面 */
} mtg_action_t;

/* 牌图加载状态 */
typedef enum {
    MTG_CARD_IMG_NONE = 0,    /* 无牌图 / 离线纯文本牌面 */
    MTG_CARD_IMG_LOADING,     /* 牌图正在下载 */
    MTG_CARD_IMG_READY,       /* 牌图已就绪 */
    MTG_CARD_IMG_FAILED,      /* 牌图下载失败，回退文本牌面 */
} mtg_card_img_state_t;

/* 完整应用状态结构 */
typedef struct {
    mtg_screen_t screen;        /* 当前显示的屏幕 */
    mtg_view_t view;            /* 当前浏览的卡牌视图 */
    mtg_net_state_t net_state;  /* 网络状态 */
    char wifi_ssid[33];         /* 当前 Wi-Fi 名称 */
    char ip_addr[16];           /* 分配到的 IP 地址 */
    int battery_soc;            /* 电池电量 (0-100, -1 为无效) */
    bool is_fetching;           /* 是否正在从网络请求卡牌 */
    bool is_online_card;        /* 当前展示的卡牌是否来自网络 */
    mtg_card_t current_card;    /* 当前展示的卡牌数据（副本） */
    size_t card_index;          /* 本地卡牌当前序号 */
    mtg_card_img_state_t img_state; /* 牌图加载状态 */
    const uint8_t *img_data;    /* 牌图 JPEG 字节缓冲指针 */
    size_t img_size;            /* 牌图字节大小 */
    uint16_t img_w;             /* 牌图宽 (px) */
    uint16_t img_h;             /* 牌图高 (px) */
} mtg_state_t;

/* 初始化状态机 */
void mtg_state_init(mtg_state_t *state);

/* 处理按键事件，返回产生的动作 */
mtg_action_t mtg_state_handle_key(mtg_state_t *state, mtg_key_t key);

/* 设置当前卡牌 */
void mtg_state_set_card(mtg_state_t *state, const mtg_card_t *card, bool is_online);

/* 更新卡牌图片数据 */
void mtg_state_set_image(mtg_state_t *state, mtg_card_img_state_t img_state,
                         const uint8_t *data, size_t size, uint16_t w, uint16_t h);

/* 更新网络连接状态 */
void mtg_state_set_net_state(mtg_state_t *state, mtg_net_state_t net_state);

/* 更新 Wi-Fi IP 与 SSID */
void mtg_state_set_net_info(mtg_state_t *state, const char *ssid, const char *ip);

/* 更新电池电量 */
void mtg_state_set_battery(mtg_state_t *state, int soc);

/* 设置网络请求中的标志 */
void mtg_state_set_fetching(mtg_state_t *state, bool fetching);

#ifdef __cplusplus
}
#endif
