#include "mtg_ui.h"
#include "mtg_mana.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

/* 外部字体库：
 * 1. mtg_font_16: 16px Noto Sans SC + Mana Font 官方符号
 * 2. mtg_font_serif_20: 20px 大字重思源宋体 Heavy (用于风味文字诗意大字展示)
 */
extern const lv_font_t mtg_font_16;
extern const lv_font_t mtg_font_serif_20;

/* UI 根组件与各区域容器 */
static lv_obj_t *s_scr = NULL;

/* 顶部状态栏 */
static lv_obj_t *s_top_bar = NULL;
static lv_obj_t *s_lbl_net = NULL;
static lv_obj_t *s_lbl_tabs = NULL;
static lv_obj_t *s_lbl_battery = NULL;

/* 中间主卡片区 */
static lv_obj_t *s_main_area = NULL;

/* 视图 1：风味文字容器 */
static lv_obj_t *s_box_flavor = NULL;
static lv_obj_t *s_lbl_flavor_quote_zh = NULL;
static lv_obj_t *s_lbl_flavor_quote_en = NULL;
static lv_obj_t *s_lbl_flavor_author = NULL;

/* 视图 2：牌面全景容器与牌图控件 */
static lv_obj_t *s_box_face = NULL;
static lv_obj_t *s_lbl_face_name = NULL;
static lv_obj_t *s_lbl_face_en = NULL;
static lv_obj_t *s_lbl_face_type = NULL;
static lv_obj_t *s_lbl_face_meta = NULL;
static lv_obj_t *s_box_face_pt = NULL;
static lv_obj_t *s_lbl_face_pt = NULL;
static lv_obj_t *s_img_card_art = NULL;
static lv_obj_t *s_lbl_face_status = NULL;
static lv_image_dsc_t s_card_img_dsc;

/* 视图 3：牌效果容器 */
static lv_obj_t *s_box_oracle = NULL;
static lv_obj_t *s_lbl_oracle_title = NULL;
static lv_obj_t *s_lbl_oracle_en = NULL;
static lv_obj_t *s_lbl_oracle_type = NULL;
static lv_obj_t *s_lbl_oracle_meta = NULL;
static lv_obj_t *s_lbl_oracle_text = NULL;

/* 配网专用界面容器 */
static lv_obj_t *s_box_provision = NULL;
static lv_obj_t *s_lbl_prov_title = NULL;
static lv_obj_t *s_lbl_prov_guide = NULL;
static lv_obj_t *s_lbl_prov_status = NULL;
static lv_obj_t *s_lbl_prov_info = NULL;

/* 底部操作指引栏 */
static lv_obj_t *s_bottom_bar = NULL;
static lv_obj_t *s_lbl_bottom_tips = NULL;

/* 抽牌加载模态卡片与动画组件 */
#define MTG_ANIM_DOTS 5

static lv_obj_t *s_box_loading = NULL;
static lv_obj_t *s_lbl_loading_title = NULL;
static lv_obj_t *s_lbl_loading_lore = NULL;
static lv_obj_t *s_box_dots = NULL;
static lv_obj_t *s_dots[MTG_ANIM_DOTS];
static lv_timer_t *s_anim_timer = NULL;
static uint32_t s_anim_step = 0;

static const char *s_loading_lores[] = {
    "正在漫游多重宇宙 ·",
    "正在漫游多重宇宙 ··",
    "正在感应法术力源 ···",
    "正在感应法术力源 ····",
    "正在探寻远古牌库 ·",
    "正在探寻远古牌库 ··",
    "正在凝聚时空火花 ···",
    "新卡牌即将显现 ····",
};
#define S_LOADING_LORES_COUNT (sizeof(s_loading_lores) / sizeof(s_loading_lores[0]))

static void update_loading_anim_frame(void)
{
    if (!s_box_loading || !s_lbl_loading_lore) return;

    /* 1. 更新动效提示文字 */
    const char *lore = s_loading_lores[s_anim_step % S_LOADING_LORES_COUNT];
    lv_label_set_text(s_lbl_loading_lore, lore);

    /* 2. 更新 5 颗发光法术力能量圆点（波浪往返移动 0->1->2->3->4->3->2->1） */
    int dot_cycle = (MTG_ANIM_DOTS - 1) * 2; /* 8 步一循环 */
    int step_in_cycle = (int)(s_anim_step % dot_cycle);
    int active_idx = (step_in_cycle < MTG_ANIM_DOTS) ? step_in_cycle : (dot_cycle - step_in_cycle);

    for (int i = 0; i < MTG_ANIM_DOTS; i++) {
        if (!s_dots[i]) continue;
        if (i == active_idx) {
            lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(0xd4af37), 0);
            lv_obj_set_style_border_color(s_dots[i], lv_color_hex(0xfff1c5), 0);
        } else {
            lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(0x21262d), 0);
            lv_obj_set_style_border_color(s_dots[i], lv_color_hex(0x30363d), 0);
        }
    }
}

static void loading_anim_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_anim_step++;
    update_loading_anim_frame();
}

/* 法术力徽章 UI 结构体（带底色圆圈渲染） */
#define MTG_MAX_BADGES 8

typedef struct {
    lv_obj_t *box;
    lv_obj_t *lbl;
} mtg_badge_ui_t;

typedef struct {
    lv_obj_t *cont;
    mtg_badge_ui_t badges[MTG_MAX_BADGES];
} mtg_mana_row_t;

static mtg_mana_row_t s_face_mana;
static mtg_mana_row_t s_oracle_mana;

static const lv_font_t *get_app_font(void)
{
    return &mtg_font_16;
}

static const lv_font_t *get_en_font(void)
{
    return &lv_font_montserrat_12;
}

/* 清洗风味文字中潜藏的 \n～ 等作者署名杂音，并提取作者 */
static void clean_flavor_quote_c(const char *in, char *out_quote, size_t quote_sz,
                                 char *out_author, size_t author_sz)
{
    if (!in || !out_quote || quote_sz == 0) return;
    out_quote[0] = '\0';
    if (out_author && author_sz > 0) out_author[0] = '\0';

    size_t in_len = strlen(in);
    if (in_len == 0) return;

    const char *author_ptr = NULL;
    size_t quote_len = in_len;

    for (size_t i = 0; i < in_len; i++) {
        if ((in[i] == '\n' || (in[i] == '\\' && in[i+1] == 'n')) && i + 1 < in_len) {
            size_t step = (in[i] == '\\') ? 2 : 1;
            const char *p = in + i + step;
            while (*p == ' ') p++;
            if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBD && (unsigned char)p[2] == 0x9E) {
                author_ptr = p + 3;
                quote_len = i;
                break;
            } else if (p[0] == '~' || p[0] == '-') {
                author_ptr = p + 1;
                quote_len = i;
                break;
            } else if ((unsigned char)p[0] == 0xE2 && (unsigned char)p[1] == 0x80 && (unsigned char)p[2] == 0x94) {
                author_ptr = p + 3;
                quote_len = i;
                break;
            }
        }
    }

    size_t copy_q = (quote_len < quote_sz - 1) ? quote_len : (quote_sz - 1);
    memcpy(out_quote, in, copy_q);
    out_quote[copy_q] = '\0';

    while (copy_q > 0 && (out_quote[copy_q - 1] == ' ' || out_quote[copy_q - 1] == '\n' || out_quote[copy_q - 1] == '\r')) {
        out_quote[--copy_q] = '\0';
    }

    if (author_ptr && out_author && author_sz > 0) {
        while (*author_ptr == ' ' || *author_ptr == '~' || *author_ptr == '-') author_ptr++;
        if (*author_ptr) {
            snprintf(out_author, author_sz, "——%s", author_ptr);
        }
    }
}

static void create_mana_row(mtg_mana_row_t *row, lv_obj_t *parent, const lv_font_t *font)
{
    row->cont = lv_obj_create(parent);
    lv_obj_set_size(row->cont, LV_SIZE_CONTENT, 20);
    lv_obj_set_style_bg_opa(row->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row->cont, 0, 0);
    lv_obj_set_style_pad_all(row->cont, 0, 0);
    lv_obj_set_style_pad_column(row->cont, 2, 0);
    lv_obj_set_flex_flow(row->cont, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(row->cont, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < MTG_MAX_BADGES; i++) {
        lv_obj_t *b = lv_obj_create(row->cont);
        lv_obj_set_size(b, 18, 18);
        lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(b, 0, 0);
        lv_obj_set_style_border_width(b, 1, 0);
        lv_obj_set_style_border_color(b, lv_color_hex(0x2a2d32), 0);
        lv_obj_set_style_clip_corner(b, true, 0);
        lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *l = lv_label_create(b);
        lv_obj_set_style_text_font(l, font, 0);
        lv_obj_set_style_pad_all(l, 0, 0);
        lv_obj_center(l);

        row->badges[i].box = b;
        row->badges[i].lbl = l;
        lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_mana_row(mtg_mana_row_t *row, const char *mana_cost)
{
    if (!row || !row->cont) return;

    mtg_mana_badge_t parsed[MTG_MAX_BADGES];
    size_t count = mtg_mana_parse_badges(mana_cost, parsed, MTG_MAX_BADGES);

    for (int i = 0; i < MTG_MAX_BADGES; i++) {
        if (i < count) {
            lv_obj_clear_flag(row->badges[i].box, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(row->badges[i].box, lv_color_hex(parsed[i].bg_color), 0);
            lv_obj_set_style_bg_opa(row->badges[i].box, LV_OPA_COVER, 0);
            lv_label_set_text(row->badges[i].lbl, parsed[i].label_utf8);
            lv_obj_set_style_text_color(row->badges[i].lbl, lv_color_hex(parsed[i].text_color), 0);
        } else {
            lv_obj_add_flag(row->badges[i].box, LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_obj_update_layout(row->cont);
}

bool mtg_ui_create(void)
{
    s_scr = lv_obj_create(NULL);
    if (!s_scr) return false;

    /* 整体背景：深度秘法黑/暗蓝 (0x0d, 0x11, 0x17) */
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);
    lv_obj_set_size(s_scr, 240, 320);
    lv_obj_clear_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    const lv_font_t *font = get_app_font();

    /* ================= 1. 顶部状态栏 (高度 24) ================= */
    s_top_bar = lv_obj_create(s_scr);
    lv_obj_set_size(s_top_bar, 240, 24);
    lv_obj_set_pos(s_top_bar, 0, 0);
    lv_obj_set_style_bg_color(s_top_bar, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_border_side(s_top_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(s_top_bar, lv_color_hex(0x8b6508), 0);
    lv_obj_set_style_border_width(s_top_bar, 1, 0);
    lv_obj_set_style_pad_all(s_top_bar, 2, 0);
    lv_obj_clear_flag(s_top_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 网络状态 */
    s_lbl_net = lv_label_create(s_top_bar);
    lv_obj_set_style_text_font(s_lbl_net, font, 0);
    lv_obj_set_style_text_color(s_lbl_net, lv_color_hex(0x8b949e), 0);
    lv_obj_align(s_lbl_net, LV_ALIGN_LEFT_MID, 4, 0);
    lv_label_set_text(s_lbl_net, "本地");

    /* 视图指示标签 */
    s_lbl_tabs = lv_label_create(s_top_bar);
    lv_obj_set_style_text_font(s_lbl_tabs, font, 0);
    lv_obj_set_style_text_color(s_lbl_tabs, lv_color_hex(0xd4af37), 0);
    lv_obj_align(s_lbl_tabs, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(s_lbl_tabs, "[风味] 牌面 效果");

    /* 电池电量 */
    s_lbl_battery = lv_label_create(s_top_bar);
    lv_obj_set_style_text_font(s_lbl_battery, font, 0);
    lv_obj_set_style_text_color(s_lbl_battery, lv_color_hex(0x7ee787), 0);
    lv_obj_align(s_lbl_battery, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_label_set_text(s_lbl_battery, "85%");

    /* ================= 2. 中间主区域 (高度 274) ================= */
    s_main_area = lv_obj_create(s_scr);
    lv_obj_set_size(s_main_area, 240, 274);
    lv_obj_set_pos(s_main_area, 0, 24);
    lv_obj_set_style_bg_opa(s_main_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_main_area, 0, 0);
    lv_obj_set_style_pad_all(s_main_area, 0, 0);
    lv_obj_clear_flag(s_main_area, LV_OBJ_FLAG_SCROLLABLE);

    /* ---------------- 视图 1：风味文字 (纯风味中英双语，沉浸式诗意展示) ---------------- */
    s_box_flavor = lv_obj_create(s_main_area);
    lv_obj_set_size(s_box_flavor, 228, 262);
    lv_obj_set_pos(s_box_flavor, 6, 6);
    lv_obj_set_style_bg_color(s_box_flavor, lv_color_hex(0x131720), 0);
    lv_obj_set_style_border_color(s_box_flavor, lv_color_hex(0x524321), 0);
    lv_obj_set_style_border_width(s_box_flavor, 1, 0);
    lv_obj_set_style_radius(s_box_flavor, 6, 0);
    lv_obj_set_style_pad_all(s_box_flavor, 8, 0);
    lv_obj_clear_flag(s_box_flavor, LV_OBJ_FLAG_SCROLLABLE);

    /* 中文风味文字 (采用大字重思源宋体 20px，视觉冲击力显著，不显示牌名/费用/类别) */
    s_lbl_flavor_quote_zh = lv_label_create(s_box_flavor);
    lv_obj_set_style_text_font(s_lbl_flavor_quote_zh, &mtg_font_serif_20, 0);
    lv_obj_set_style_text_color(s_lbl_flavor_quote_zh, lv_color_hex(0xf8edd8), 0);
    lv_obj_set_style_text_line_space(s_lbl_flavor_quote_zh, 6, 0);
    lv_obj_set_width(s_lbl_flavor_quote_zh, 212);
    lv_label_set_long_mode(s_lbl_flavor_quote_zh, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_flavor_quote_zh, LV_ALIGN_TOP_LEFT, 0, 14);
    lv_label_set_text(s_lbl_flavor_quote_zh, "“思考电光，你便成灰。”");

    /* 英文风味文字原文 (采用更精致的小字号 12px Montserrat，行距紧凑，杜绝交叠) */
    s_lbl_flavor_quote_en = lv_label_create(s_box_flavor);
    lv_obj_set_style_text_font(s_lbl_flavor_quote_en, get_en_font(), 0);
    lv_obj_set_style_text_color(s_lbl_flavor_quote_en, lv_color_hex(0x9aa5b1), 0);
    lv_obj_set_style_text_line_space(s_lbl_flavor_quote_en, 3, 0);
    lv_obj_set_width(s_lbl_flavor_quote_en, 212);
    lv_label_set_long_mode(s_lbl_flavor_quote_en, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_flavor_quote_en, LV_ALIGN_TOP_LEFT, 0, 90);
    lv_label_set_text(s_lbl_flavor_quote_en, "\"Think lightning, and you are ash.\"");

    /* 引言人物/出处署名 (金黄色典雅出处) */
    s_lbl_flavor_author = lv_label_create(s_box_flavor);
    lv_obj_set_style_text_font(s_lbl_flavor_author, font, 0);
    lv_obj_set_style_text_color(s_lbl_flavor_author, lv_color_hex(0xd4af37), 0);
    lv_obj_set_width(s_lbl_flavor_author, 210);
    lv_obj_set_style_text_align(s_lbl_flavor_author, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(s_lbl_flavor_author, LV_ALIGN_BOTTOM_RIGHT, 0, -8);
    lv_label_set_text(s_lbl_flavor_author, "——多明纳里亚谚语");

    /* ---------------- 视图 2：牌面全景 (离线文本牌面 / 联网官方全图) ---------------- */
    s_box_face = lv_obj_create(s_main_area);
    lv_obj_set_size(s_box_face, 240, 274);
    lv_obj_set_pos(s_box_face, 0, 0);
    lv_obj_set_style_bg_opa(s_box_face, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_box_face, 0, 0);
    lv_obj_set_style_pad_all(s_box_face, 0, 0);
    lv_obj_clear_flag(s_box_face, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_face_name = lv_label_create(s_box_face);
    lv_obj_set_style_text_font(s_lbl_face_name, font, 0);
    lv_obj_set_style_text_color(s_lbl_face_name, lv_color_hex(0xf0e6d2), 0);
    lv_obj_align(s_lbl_face_name, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_width(s_lbl_face_name, 125);
    lv_label_set_long_mode(s_lbl_face_name, LV_LABEL_LONG_DOT);
    lv_label_set_text(s_lbl_face_name, "闪电击");

    /* 右上角带底色法术力徽章 */
    create_mana_row(&s_face_mana, s_box_face, font);
    lv_obj_align(s_face_mana.cont, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* 英文原名：采用 12px Montserrat 小字号，纵向预留间距，杜绝中英文交叠 */
    s_lbl_face_en = lv_label_create(s_box_face);
    lv_obj_set_style_text_font(s_lbl_face_en, get_en_font(), 0);
    lv_obj_set_style_text_color(s_lbl_face_en, lv_color_hex(0x8b949e), 0);
    lv_obj_set_width(s_lbl_face_en, 125);
    lv_label_set_long_mode(s_lbl_face_en, LV_LABEL_LONG_DOT);
    lv_obj_align(s_lbl_face_en, LV_ALIGN_TOP_LEFT, 0, 24);
    lv_label_set_text(s_lbl_face_en, "Lightning Bolt");

    s_lbl_face_type = lv_label_create(s_box_face);
    lv_obj_set_style_text_font(s_lbl_face_type, font, 0);
    lv_obj_set_style_text_color(s_lbl_face_type, lv_color_hex(0xd4af37), 0);
    lv_obj_align(s_lbl_face_type, LV_ALIGN_TOP_LEFT, 0, 46);
    lv_label_set_text(s_lbl_face_type, "瞬间");

    s_lbl_face_meta = lv_label_create(s_box_face);
    lv_obj_set_style_text_font(s_lbl_face_meta, font, 0);
    lv_obj_set_style_text_color(s_lbl_face_meta, lv_color_hex(0x6e7681), 0);
    lv_obj_align(s_lbl_face_meta, LV_ALIGN_TOP_LEFT, 0, 72);
    lv_label_set_text(s_lbl_face_meta, "系列：LEA #161");

    /* 右下角攻防徽章 */
    s_box_face_pt = lv_obj_create(s_box_face);
    lv_obj_set_size(s_box_face_pt, 64, 30);
    lv_obj_align(s_box_face_pt, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(s_box_face_pt, lv_color_hex(0x21262d), 0);
    lv_obj_set_style_border_color(s_box_face_pt, lv_color_hex(0xe5c07b), 0);
    lv_obj_set_style_border_width(s_box_face_pt, 1, 0);
    lv_obj_set_style_radius(s_box_face_pt, 4, 0);
    lv_obj_set_style_pad_all(s_box_face_pt, 2, 0);

    s_lbl_face_pt = lv_label_create(s_box_face_pt);
    lv_obj_set_style_text_font(s_lbl_face_pt, font, 0);
    lv_obj_set_style_text_color(s_lbl_face_pt, lv_color_hex(0xe5c07b), 0);
    lv_obj_center(s_lbl_face_pt);
    lv_label_set_text(s_lbl_face_pt, "10/10");

    /* 居中牌图显示控件（联网时全尺寸展示 Scryfall 高清牌图） */
    s_img_card_art = lv_image_create(s_box_face);
    lv_obj_align(s_img_card_art, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(s_img_card_art, LV_OBJ_FLAG_HIDDEN);

    /* 联网牌图加载状态提示标签 */
    s_lbl_face_status = lv_label_create(s_box_face);
    lv_obj_set_style_text_font(s_lbl_face_status, font, 0);
    lv_obj_set_style_text_color(s_lbl_face_status, lv_color_hex(0xe5c07b), 0);
    lv_obj_set_style_text_align(s_lbl_face_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_lbl_face_status);
    lv_obj_add_flag(s_lbl_face_status, LV_OBJ_FLAG_HIDDEN);

    /* ---------------- 视图 3：牌效果 (Oracle + Mana Font) ---------------- */
    s_box_oracle = lv_obj_create(s_main_area);
    lv_obj_set_size(s_box_oracle, 228, 262);
    lv_obj_set_pos(s_box_oracle, 6, 6);
    lv_obj_set_style_bg_color(s_box_oracle, lv_color_hex(0x131720), 0);
    lv_obj_set_style_border_color(s_box_oracle, lv_color_hex(0x524321), 0);
    lv_obj_set_style_border_width(s_box_oracle, 1, 0);
    lv_obj_set_style_radius(s_box_oracle, 6, 0);
    lv_obj_set_style_pad_all(s_box_oracle, 8, 0);

    s_lbl_oracle_title = lv_label_create(s_box_oracle);
    lv_obj_set_style_text_font(s_lbl_oracle_title, font, 0);
    lv_obj_set_style_text_color(s_lbl_oracle_title, lv_color_hex(0xe5c07b), 0);
    lv_obj_align(s_lbl_oracle_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_width(s_lbl_oracle_title, 130);
    lv_label_set_long_mode(s_lbl_oracle_title, LV_LABEL_LONG_DOT);
    lv_label_set_text(s_lbl_oracle_title, "闪电击");

    /* 英文原名 */
    s_lbl_oracle_en = lv_label_create(s_box_oracle);
    lv_obj_set_style_text_font(s_lbl_oracle_en, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lbl_oracle_en, lv_color_hex(0x8b949e), 0);
    lv_obj_align(s_lbl_oracle_en, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_obj_set_width(s_lbl_oracle_en, 210);
    lv_label_set_long_mode(s_lbl_oracle_en, LV_LABEL_LONG_DOT);
    lv_label_set_text(s_lbl_oracle_en, "Lightning Bolt");

    /* 右上角带底色法术力徽章 */
    create_mana_row(&s_oracle_mana, s_box_oracle, font);
    lv_obj_align(s_oracle_mana.cont, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* 类别 */
    s_lbl_oracle_type = lv_label_create(s_box_oracle);
    lv_obj_set_style_text_font(s_lbl_oracle_type, font, 0);
    lv_obj_set_style_text_color(s_lbl_oracle_type, lv_color_hex(0xd4af37), 0);
    lv_obj_align(s_lbl_oracle_type, LV_ALIGN_TOP_LEFT, 0, 38);
    lv_label_set_text(s_lbl_oracle_type, "瞬间");

    /* 系列 + 编号元信息出处 */
    s_lbl_oracle_meta = lv_label_create(s_box_oracle);
    lv_obj_set_style_text_font(s_lbl_oracle_meta, font, 0);
    lv_obj_set_style_text_color(s_lbl_oracle_meta, lv_color_hex(0x6e7681), 0);
    lv_obj_align(s_lbl_oracle_meta, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_label_set_text(s_lbl_oracle_meta, "系列：LEA #161");

    /* 规则叙述：位于元信息出处下方 */
    s_lbl_oracle_text = lv_label_create(s_box_oracle);
    lv_obj_set_style_text_font(s_lbl_oracle_text, font, 0);
    lv_obj_set_style_text_color(s_lbl_oracle_text, lv_color_hex(0xf0e6d2), 0);
    lv_obj_set_style_text_line_space(s_lbl_oracle_text, 4, 0);
    lv_obj_set_width(s_lbl_oracle_text, 210);
    lv_label_set_long_mode(s_lbl_oracle_text, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_oracle_text, LV_ALIGN_TOP_LEFT, 0, 84);
    lv_label_set_text(s_lbl_oracle_text, "闪电击对任意一个目标造成3点伤害。");

    /* ---------------- 4. 配网页面容器 (覆盖全主区) ---------------- */
    s_box_provision = lv_obj_create(s_main_area);
    lv_obj_set_size(s_box_provision, 228, 262);
    lv_obj_set_pos(s_box_provision, 6, 6);
    lv_obj_set_style_bg_color(s_box_provision, lv_color_hex(0x131720), 0);
    lv_obj_set_style_border_color(s_box_provision, lv_color_hex(0x388bfd), 0);
    lv_obj_set_style_border_width(s_box_provision, 1, 0);
    lv_obj_set_style_radius(s_box_provision, 6, 0);
    lv_obj_set_style_pad_all(s_box_provision, 8, 0);
    lv_obj_add_flag(s_box_provision, LV_OBJ_FLAG_HIDDEN);

    s_lbl_prov_title = lv_label_create(s_box_provision);
    lv_obj_set_style_text_font(s_lbl_prov_title, font, 0);
    lv_obj_set_style_text_color(s_lbl_prov_title, lv_color_hex(0x58a6ff), 0);
    lv_obj_align(s_lbl_prov_title, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(s_lbl_prov_title, "Wi-Fi 热点网页配网");

    s_lbl_prov_guide = lv_label_create(s_box_provision);
    lv_obj_set_style_text_font(s_lbl_prov_guide, font, 0);
    lv_obj_set_style_text_color(s_lbl_prov_guide, lv_color_hex(0xc9d1d9), 0);
    lv_obj_set_width(s_lbl_prov_guide, 210);
    lv_label_set_long_mode(s_lbl_prov_guide, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_prov_guide, LV_ALIGN_TOP_LEFT, 0, 26);
    lv_label_set_text(s_lbl_prov_guide,
                      "1. 手机连接 Wi-Fi 热点：\n   FoloPassport-WiFi\n"
                      "2. 浏览器打开网页：\n   http://192.168.4.1\n"
                      "3. 选择家中 Wi-Fi 并输入密码。");

    s_lbl_prov_status = lv_label_create(s_box_provision);
    lv_obj_set_style_text_font(s_lbl_prov_status, font, 0);
    lv_obj_set_style_text_color(s_lbl_prov_status, lv_color_hex(0xe5c07b), 0);
    lv_obj_set_width(s_lbl_prov_status, 210);
    lv_label_set_long_mode(s_lbl_prov_status, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_prov_status, LV_ALIGN_TOP_LEFT, 0, 140);
    lv_label_set_text(s_lbl_prov_status, "当前状态：等待连接");

    s_lbl_prov_info = lv_label_create(s_box_provision);
    lv_obj_set_style_text_font(s_lbl_prov_info, font, 0);
    lv_obj_set_style_text_color(s_lbl_prov_info, lv_color_hex(0x7ee787), 0);
    lv_obj_set_width(s_lbl_prov_info, 210);
    lv_label_set_long_mode(s_lbl_prov_info, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_prov_info, LV_ALIGN_TOP_LEFT, 0, 175);
    lv_label_set_text(s_lbl_prov_info, "IP：未分配");

    lv_obj_t *lbl_prov_tip = lv_label_create(s_box_provision);
    lv_obj_set_style_text_font(lbl_prov_tip, font, 0);
    lv_obj_set_style_text_color(lbl_prov_tip, lv_color_hex(0x8b949e), 0);
    lv_obj_align(lbl_prov_tip, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text(lbl_prov_tip, "[按 OK 键返回浏览]");

    /* ---------------- 5. 抽牌中模态浮动卡片 (216x104，居中覆盖主区域) ---------------- */
    s_box_loading = lv_obj_create(s_main_area);
    lv_obj_set_size(s_box_loading, 216, 104);
    lv_obj_center(s_box_loading);
    lv_obj_set_style_bg_color(s_box_loading, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_bg_opa(s_box_loading, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_box_loading, lv_color_hex(0xd4af37), 0);
    lv_obj_set_style_border_width(s_box_loading, 2, 0);
    lv_obj_set_style_radius(s_box_loading, 10, 0);
    lv_obj_set_style_pad_all(s_box_loading, 6, 0);
    lv_obj_clear_flag(s_box_loading, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_box_loading, LV_OBJ_FLAG_HIDDEN);

    s_lbl_loading_title = lv_label_create(s_box_loading);
    lv_obj_set_style_text_font(s_lbl_loading_title, font, 0);
    lv_obj_set_style_text_color(s_lbl_loading_title, lv_color_hex(0xf8edd8), 0);
    lv_obj_align(s_lbl_loading_title, LV_ALIGN_TOP_MID, 0, 4);
    lv_label_set_text(s_lbl_loading_title, "【时空抽牌中】");

    s_lbl_loading_lore = lv_label_create(s_box_loading);
    lv_obj_set_style_text_font(s_lbl_loading_lore, font, 0);
    lv_obj_set_style_text_color(s_lbl_loading_lore, lv_color_hex(0x58a6ff), 0);
    lv_obj_align(s_lbl_loading_lore, LV_ALIGN_TOP_MID, 0, 32);
    lv_label_set_text(s_lbl_loading_lore, "正在漫游多重宇宙 ·");

    s_box_dots = lv_obj_create(s_box_loading);
    lv_obj_set_size(s_box_dots, LV_SIZE_CONTENT, 14);
    lv_obj_align(s_box_dots, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_opa(s_box_dots, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_box_dots, 0, 0);
    lv_obj_set_style_pad_all(s_box_dots, 0, 0);
    lv_obj_set_style_pad_column(s_box_dots, 8, 0);
    lv_obj_set_flex_flow(s_box_dots, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(s_box_dots, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < MTG_ANIM_DOTS; i++) {
        s_dots[i] = lv_obj_create(s_box_dots);
        lv_obj_set_size(s_dots[i], 10, 10);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_dots[i], 1, 0);
        lv_obj_set_style_border_color(s_dots[i], lv_color_hex(0x30363d), 0);
        lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(0x21262d), 0);
        lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
        lv_obj_clear_flag(s_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    s_anim_timer = lv_timer_create(loading_anim_timer_cb, 200, NULL);
    if (s_anim_timer) {
        lv_timer_pause(s_anim_timer);
    }

    /* ================= 3. 底部快捷提示条 (高度 22) ================= */
    s_bottom_bar = lv_obj_create(s_scr);
    lv_obj_set_size(s_bottom_bar, 240, 22);
    lv_obj_set_pos(s_bottom_bar, 0, 298);
    lv_obj_set_style_bg_color(s_bottom_bar, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_border_side(s_bottom_bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(s_bottom_bar, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_border_width(s_bottom_bar, 1, 0);
    lv_obj_set_style_pad_all(s_bottom_bar, 1, 0);
    lv_obj_clear_flag(s_bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_bottom_tips = lv_label_create(s_bottom_bar);
    lv_obj_set_style_text_font(s_lbl_bottom_tips, font, 0);
    lv_obj_set_style_text_color(s_lbl_bottom_tips, lv_color_hex(0x8b949e), 0);
    lv_obj_center(s_lbl_bottom_tips);
    lv_label_set_text(s_lbl_bottom_tips, "UP/DN切视图 | OK抽牌 | 长按配网");

    lv_screen_load(s_scr);
    return true;
}

void mtg_ui_refresh(const mtg_state_t *state)
{
    if (!state || !s_scr) return;

    /* 1. 刷新网络状态小标 */
    if (state->net_state == MTG_NET_CONNECTED) {
        lv_label_set_text(s_lbl_net, "Wi-Fi");
        lv_obj_set_style_text_color(s_lbl_net, lv_color_hex(0x7ee787), 0);
    } else if (state->net_state == MTG_NET_CONNECTING) {
        lv_label_set_text(s_lbl_net, "连接中");
        lv_obj_set_style_text_color(s_lbl_net, lv_color_hex(0xe5c07b), 0);
    } else if (state->net_state == MTG_NET_PROV_ACTIVE) {
        lv_label_set_text(s_lbl_net, "AP配网");
        lv_obj_set_style_text_color(s_lbl_net, lv_color_hex(0x58a6ff), 0);
    } else {
        lv_label_set_text(s_lbl_net, "离线");
        lv_obj_set_style_text_color(s_lbl_net, lv_color_hex(0x8b949e), 0);
    }

    /* 2. 刷新电池电量 */
    if (state->battery_soc >= 0) {
        char bat_buf[16];
        snprintf(bat_buf, sizeof(bat_buf), "%d%%", state->battery_soc);
        lv_label_set_text(s_lbl_battery, bat_buf);
        lv_obj_clear_flag(s_lbl_battery, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_lbl_battery, LV_OBJ_FLAG_HIDDEN);
    }

    /* 3. 页面模式判断 */
    if (state->screen == MTG_SCREEN_PROVISION) {
        lv_obj_add_flag(s_box_loading, LV_OBJ_FLAG_HIDDEN);
        if (s_anim_timer) {
            lv_timer_pause(s_anim_timer);
        }
        lv_obj_clear_flag(s_box_provision, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_flavor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_face, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_oracle, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(s_lbl_tabs, "【配网模式】");
        lv_label_set_text(s_lbl_bottom_tips, "请在手机完成配网，按 OK 返回");

        if (state->net_state == MTG_NET_CONNECTED) {
            lv_label_set_text(s_lbl_prov_status, "状态：Wi-Fi 已连接成功！");
            char info_buf[128];
            snprintf(info_buf, sizeof(info_buf), "SSID: %s\nIP: %s\n\n即将进入万智牌世界...",
                     state->wifi_ssid[0] ? state->wifi_ssid : "已连接",
                     state->ip_addr[0] ? state->ip_addr : "192.168.x.x");
            lv_label_set_text(s_lbl_prov_info, info_buf);
            lv_label_set_text(s_lbl_bottom_tips, "配网成功！即将进入浏览...");
        } else if (state->net_state == MTG_NET_CONNECTING) {
            lv_label_set_text(s_lbl_prov_status, "状态：正在连接 Wi-Fi...");
            char info_buf[96];
            snprintf(info_buf, sizeof(info_buf), "目标: %s\n正在等待路由器分配 IP...",
                     state->wifi_ssid[0] ? state->wifi_ssid : "Wi-Fi");
            lv_label_set_text(s_lbl_prov_info, info_buf);
        } else {
            lv_label_set_text(s_lbl_prov_status, "状态：热点已开启 (等待网页配网)");
            lv_label_set_text(s_lbl_prov_info, "热点: FoloPassport-WiFi\n网页: 192.168.4.1");
        }
        return;
    }

    /* 主浏览页面 */
    lv_obj_add_flag(s_box_provision, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_lbl_bottom_tips, "UP/DN切视图 | OK抽牌 | 长按配网");

    bool is_online = (state->net_state == MTG_NET_CONNECTED);
    const char *mid_tab = is_online ? "牌图" : "牌面";

    if (state->view == MTG_VIEW_FLAVOR) {
        char tab_buf[32];
        snprintf(tab_buf, sizeof(tab_buf), "[风味] %s 效果", mid_tab);
        lv_label_set_text(s_lbl_tabs, tab_buf);
        lv_obj_clear_flag(s_box_flavor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_face, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_oracle, LV_OBJ_FLAG_HIDDEN);
    } else if (state->view == MTG_VIEW_FACE) {
        char tab_buf[32];
        snprintf(tab_buf, sizeof(tab_buf), "风味 [%s] 效果", mid_tab);
        lv_label_set_text(s_lbl_tabs, tab_buf);
        lv_obj_add_flag(s_box_flavor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_box_face, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_oracle, LV_OBJ_FLAG_HIDDEN);
    } else {
        char tab_buf[32];
        snprintf(tab_buf, sizeof(tab_buf), "风味 %s [效果]", mid_tab);
        lv_label_set_text(s_lbl_tabs, tab_buf);
        lv_obj_add_flag(s_box_flavor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_face, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_box_oracle, LV_OBJ_FLAG_HIDDEN);
    }

    const mtg_card_t *c = &state->current_card;

    /* 1. 风味视图更新 (只显示中英文风味文字，彻底去除牌名/费用/类别等冗余信息) */
    char clean_zh[384] = {0};
    char clean_author[64] = {0};
    clean_flavor_quote_c(c->flavor_text ? c->flavor_text : "", clean_zh, sizeof(clean_zh),
                         clean_author, sizeof(clean_author));
    if (!clean_author[0] && c->flavor_author && c->flavor_author[0]) {
        snprintf(clean_author, sizeof(clean_author), "%s", c->flavor_author);
    }

    lv_label_set_text(s_lbl_flavor_quote_zh, clean_zh);
    lv_obj_set_pos(s_lbl_flavor_quote_zh, 0, 14);
    lv_obj_update_layout(s_lbl_flavor_quote_zh);
    lv_coord_t zh_h = lv_obj_get_height(s_lbl_flavor_quote_zh);

    /* 英文风味文字原文 (采用 12px Montserrat 小字号，紧随中文下方，空间充裕不交叠) */
    if (c->flavor_en && c->flavor_en[0]) {
        char clean_en[384] = {0};
        clean_flavor_quote_c(c->flavor_en, clean_en, sizeof(clean_en), NULL, 0);
        lv_label_set_text(s_lbl_flavor_quote_en, clean_en);
        lv_obj_set_pos(s_lbl_flavor_quote_en, 0, 14 + zh_h + 12);
        lv_obj_clear_flag(s_lbl_flavor_quote_en, LV_OBJ_FLAG_HIDDEN);
        lv_obj_update_layout(s_lbl_flavor_quote_en);
        lv_coord_t en_h = lv_obj_get_height(s_lbl_flavor_quote_en);

        if (clean_author[0]) {
            lv_label_set_text(s_lbl_flavor_author, clean_author);
            lv_coord_t auth_y = 14 + zh_h + 12 + en_h + 10;
            if (auth_y > 230) auth_y = 230;
            lv_obj_set_pos(s_lbl_flavor_author, 0, auth_y);
            lv_obj_clear_flag(s_lbl_flavor_author, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_lbl_flavor_author, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(s_lbl_flavor_quote_en, LV_OBJ_FLAG_HIDDEN);
        if (clean_author[0]) {
            lv_label_set_text(s_lbl_flavor_author, clean_author);
            lv_coord_t auth_y = 14 + zh_h + 12;
            if (auth_y > 230) auth_y = 230;
            lv_obj_set_pos(s_lbl_flavor_author, 0, auth_y);
            lv_obj_clear_flag(s_lbl_flavor_author, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_lbl_flavor_author, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 2. 牌面 / 牌图视图更新 (联网大画幅高清插画 / 离线古典图文字卡牌) */
    if (is_online) {
        /* 联网大画幅牌图模式：容器铺满中间主视区，消除所有内边距与外边框，全尺寸呈现高清插画 */
        lv_obj_set_size(s_box_face, 240, 274);
        lv_obj_set_pos(s_box_face, 0, 0);
        lv_obj_set_style_pad_all(s_box_face, 0, 0);
        lv_obj_set_style_border_width(s_box_face, 0, 0);
        lv_obj_set_style_bg_opa(s_box_face, LV_OPA_TRANSP, 0);

        /* 隐藏全部纯文字牌面组件 */
        lv_obj_add_flag(s_lbl_face_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_lbl_face_en, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_lbl_face_type, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_lbl_face_meta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_box_face_pt, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_face_mana.cont, LV_OBJ_FLAG_HIDDEN);

        if (state->img_state == MTG_CARD_IMG_READY &&
            state->img_data != NULL && state->img_size > 0) {
            /* 牌图准备就绪：全图居中呈现 (194x270，上下仅留2px安全边距，四周黑边最小化) */
            lv_obj_add_flag(s_lbl_face_status, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_bottom_tips, "UP/DN切视图 | OK抽牌 | 长按配网");

            memset(&s_card_img_dsc, 0, sizeof(s_card_img_dsc));
            s_card_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
            s_card_img_dsc.header.cf = LV_COLOR_FORMAT_RAW;
            s_card_img_dsc.header.w = state->img_w ? state->img_w : 194;
            s_card_img_dsc.header.h = state->img_h ? state->img_h : 270;
            s_card_img_dsc.data_size = (uint32_t)state->img_size;
            s_card_img_dsc.data = state->img_data;

            lv_image_set_src(s_img_card_art, &s_card_img_dsc);
            lv_obj_center(s_img_card_art);
            lv_obj_clear_flag(s_img_card_art, LV_OBJ_FLAG_HIDDEN);
        } else if (state->img_state == MTG_CARD_IMG_LOADING) {
            lv_obj_add_flag(s_img_card_art, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_face_status, "正在获取官方高清牌图...\n请稍候");
            lv_obj_center(s_lbl_face_status);
            lv_obj_clear_flag(s_lbl_face_status, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_bottom_tips, "正在下载牌图 | UP/DN切页");
        } else {
            /* 失败或未开始 */
            lv_obj_add_flag(s_img_card_art, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_face_status, "牌图尚未下载\n[按 OK 重新获取]\n[按 UP/DN 可切页]");
            lv_obj_center(s_lbl_face_status);
            lv_obj_clear_flag(s_lbl_face_status, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_bottom_tips, "OK重试下载牌图 | UP/DN切页抽牌");
        }
    } else {
        /* 离线文字牌面模式：恢复典雅金色卡牌边框与标准内边距 */
        lv_obj_set_size(s_box_face, 228, 262);
        lv_obj_set_pos(s_box_face, 6, 6);
        lv_obj_set_style_bg_color(s_box_face, lv_color_hex(0x131720), 0);
        lv_obj_set_style_bg_opa(s_box_face, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_box_face, lv_color_hex(0x524321), 0);
        lv_obj_set_style_border_width(s_box_face, 1, 0);
        lv_obj_set_style_radius(s_box_face, 6, 0);
        lv_obj_set_style_pad_all(s_box_face, 8, 0);

        lv_obj_add_flag(s_img_card_art, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_lbl_face_status, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(s_lbl_face_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_lbl_face_en, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_lbl_face_type, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_lbl_face_meta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_face_mana.cont, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(s_lbl_face_name, c->name_zh);
        update_mana_row(&s_face_mana, c->mana_cost);
        lv_obj_set_pos(s_lbl_face_name, 0, 0);
        lv_obj_update_layout(s_lbl_face_name);
        lv_coord_t fn_h = lv_obj_get_height(s_lbl_face_name);

        /* 英文名：小字号位于中文名下方，绝不与中文名交叠 */
        lv_coord_t en_y = (fn_h > 20) ? (fn_h + 4) : 24;
        lv_label_set_text(s_lbl_face_en, c->name_en);
        lv_obj_set_pos(s_lbl_face_en, 0, en_y);
        lv_obj_update_layout(s_lbl_face_en);
        lv_coord_t fe_h = lv_obj_get_height(s_lbl_face_en);

        /* 类别：位于英文名下方 */
        lv_coord_t type_y = en_y + ((fe_h > 14) ? fe_h : 16) + 8;
        lv_label_set_text(s_lbl_face_type, c->type_line ? c->type_line : "");
        lv_obj_set_pos(s_lbl_face_type, 0, type_y);
        lv_obj_update_layout(s_lbl_face_type);
        lv_coord_t ft_h = lv_obj_get_height(s_lbl_face_type);

        /* 系列：位于类别下方 */
        lv_coord_t meta_y = type_y + ((ft_h > 18) ? ft_h : 20) + 8;
        char face_meta[80];
        snprintf(face_meta, sizeof(face_meta), "系列：%s · [离线牌面]",
                 c->set_code ? c->set_code : "");
        lv_label_set_text(s_lbl_face_meta, face_meta);
        lv_obj_set_pos(s_lbl_face_meta, 0, meta_y);

        if (c->pt && c->pt[0]) {
            lv_label_set_text(s_lbl_face_pt, c->pt);
            lv_obj_clear_flag(s_box_face_pt, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_box_face_pt, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 3. 牌效果视图更新 (牌名、英文原名、类别、系列出处与叙述纵向动态布局，绝不相互交叠) */
    lv_label_set_text(s_lbl_oracle_title, c->name_zh ? c->name_zh : "未知卡牌");
    update_mana_row(&s_oracle_mana, c->mana_cost);
    lv_obj_set_pos(s_lbl_oracle_title, 0, 0);
    lv_obj_update_layout(s_lbl_oracle_title);
    lv_coord_t ot_h = lv_obj_get_height(s_lbl_oracle_title);

    /* 英文原名紧随中文牌名下方 */
    lv_coord_t o_en_y = (ot_h > 18) ? (ot_h + 2) : 22;
    if (c->name_en && c->name_en[0]) {
        lv_label_set_text(s_lbl_oracle_en, c->name_en);
        lv_obj_set_pos(s_lbl_oracle_en, 0, o_en_y);
        lv_obj_clear_flag(s_lbl_oracle_en, LV_OBJ_FLAG_HIDDEN);
        lv_obj_update_layout(s_lbl_oracle_en);
        o_en_y += lv_obj_get_height(s_lbl_oracle_en) + 4;
    } else {
        lv_obj_add_flag(s_lbl_oracle_en, LV_OBJ_FLAG_HIDDEN);
        o_en_y = ot_h + 4;
    }

    /* 类别位于英文名下方 */
    lv_label_set_text(s_lbl_oracle_type, c->type_line ? c->type_line : "");
    lv_obj_set_pos(s_lbl_oracle_type, 0, o_en_y);
    lv_obj_update_layout(s_lbl_oracle_type);
    lv_coord_t otp_h = lv_obj_get_height(s_lbl_oracle_type);

    /* 系列 + 编号元信息出处 */
    lv_coord_t o_meta_y = o_en_y + ((otp_h > 18) ? otp_h : 20) + 4;
    char o_meta_buf[64] = {0};
    if (c->set_code && c->set_code[0]) {
        snprintf(o_meta_buf, sizeof(o_meta_buf), "系列：%s", c->set_code);
    } else {
        snprintf(o_meta_buf, sizeof(o_meta_buf), "经典万智牌池");
    }
    lv_label_set_text(s_lbl_oracle_meta, o_meta_buf);
    lv_obj_set_pos(s_lbl_oracle_meta, 0, o_meta_y);
    lv_obj_update_layout(s_lbl_oracle_meta);
    lv_coord_t om_h = lv_obj_get_height(s_lbl_oracle_meta);

    /* 效果叙述位于元信息出处下方 */
    lv_coord_t o_text_y = o_meta_y + ((om_h > 14) ? om_h : 16) + 6;
    char formatted_oracle[512] = {0};
    if (c->oracle_text && c->oracle_text[0]) {
        mtg_mana_format_text(c->oracle_text, formatted_oracle, sizeof(formatted_oracle));
        lv_label_set_text(s_lbl_oracle_text, formatted_oracle);
    } else {
        lv_label_set_text(s_lbl_oracle_text, "（无规则描述）");
    }
    lv_obj_set_pos(s_lbl_oracle_text, 0, o_text_y);

    /* 4. 抽牌互锁与动态加载卡片显隐处理 (置于最顶层，覆盖主区域) */
    if (state->is_fetching && state->screen != MTG_SCREEN_PROVISION) {
        lv_obj_move_foreground(s_box_loading);
        lv_obj_clear_flag(s_box_loading, LV_OBJ_FLAG_HIDDEN);
        if (s_anim_timer) {
            lv_timer_resume(s_anim_timer);
        }
        update_loading_anim_frame();
        lv_label_set_text(s_lbl_bottom_tips, "【正在抽牌中】请稍候...");
    } else {
        lv_obj_add_flag(s_box_loading, LV_OBJ_FLAG_HIDDEN);
        if (s_anim_timer) {
            lv_timer_pause(s_anim_timer);
        }
        s_anim_step = 0;
    }
}

void mtg_ui_destroy(void)
{
    if (s_anim_timer) {
        lv_timer_delete(s_anim_timer);
        s_anim_timer = NULL;
    }
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
}
