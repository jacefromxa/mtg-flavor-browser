#include "mtg_fetch.h"
#include "mtg_wifi_prov.h"
#include "mtg_cards.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_random.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char *TAG = "mtg_fetch";

/* 在线卡牌动态字符串驻留区（单例持久化，卡牌指针安全借用） */
typedef struct {
    char name_zh[64];
    char name_en[64];
    char mana_cost[32];
    char type_line[64];
    char oracle_text[512];
    char flavor_text[384];
    char flavor_en[384];
    char flavor_author[64];
    char pt[16];
    char set_code[32];
    char image_url[256];
} mtg_online_storage_t;

static mtg_online_storage_t s_online_storage;

/* HTTP 流式数据累加器 */
typedef struct {
    char *buffer;
    size_t max_size;
    size_t length;
    bool is_image;
    char location[256]; /* 记录 3xx 重定向的目标 Location URL */
} http_collector_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_collector_t *collector = (http_collector_t *)evt->user_data;
    if (!collector) return ESP_OK;

    switch (evt->event_id) {
    case HTTP_EVENT_ON_CONNECTED:
        /* 每次建立新连接时重置累加缓存，杜绝残留任何重定向头或前置数据 */
        if (collector->buffer) {
            collector->length = 0;
            collector->buffer[0] = '\0';
        }
        break;

    case HTTP_EVENT_ON_HEADER:
        /* 若遇到 Location 响应头，保存重定向目标 URL */
        if (evt->header_key && evt->header_value) {
            if (strcasecmp(evt->header_key, "Location") == 0) {
                snprintf(collector->location, sizeof(collector->location), "%s", evt->header_value);
                ESP_LOGI(TAG, "捕获重定向 Location: %s", collector->location);
            }
        }
        break;

    case HTTP_EVENT_ON_DATA: {
        if (!collector->buffer) return ESP_OK;
        const uint8_t *data = (const uint8_t *)evt->data;
        int data_len = evt->data_len;
        if (data_len <= 0) return ESP_OK;

        if (collector->length + data_len < collector->max_size) {
            memcpy(collector->buffer + collector->length, data, data_len);
            collector->length += data_len;
            collector->buffer[collector->length] = '\0';
        }
        break;
    }

    default:
        break;
    }
    return ESP_OK;
}

/* 解析 JPEG 数据流头部并提取图像像素宽高及是否为 Baseline JPEG */
static bool parse_jpeg_dimensions(const uint8_t *data, size_t len, uint16_t *w, uint16_t *h, bool *is_baseline)
{
    if (!data || len < 10) return false;
    /* 必须以 SOI (0xFF, 0xD8) 起始 */
    if (data[0] != 0xFF || data[1] != 0xD8) return false;

    size_t i = 2;
    while (i + 8 < len) {
        if (data[i] != 0xFF) {
            i++;
            continue;
        }
        uint8_t marker = data[i + 1];
        if (marker == 0xD9 || marker == 0xDA) {
            /* 遇到 EOI 或 SOS（扫描数据开始），头部段落结束 */
            break;
        }
        uint16_t seg_len = ((uint16_t)data[i + 2] << 8) | (uint16_t)data[i + 3];
        if (seg_len < 2 || i + 2 + seg_len > len) break;

        /* SOF0 (0xC0: Baseline), SOF1 (0xC1: Extended), SOF2 (0xC2: Progressive) */
        if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
            if (h) *h = ((uint16_t)data[i + 5] << 8) | (uint16_t)data[i + 6];
            if (w) *w = ((uint16_t)data[i + 7] << 8) | (uint16_t)data[i + 8];
            if (is_baseline) *is_baseline = (marker == 0xC0);
            return true;
        }
        i += 2 + seg_len;
    }
    return false;
}

/* 确保 JPEG 数据前 10 字节严格具备 LVGL TJPGD 识别所必须的 JFIF APP0 签名:
 * LVGL lv_tjpgd.c 中 `is_jpg()` 源码规定:
 * {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46} ("JFIF")
 * 若转码 CDN 生成的标准 JPEG 省略了该可选 marker，LVGL 会直接判定不是 JPG 而不予解码渲染！
 */
static void ensure_jfif_header(uint8_t *buf, size_t *inout_len, size_t max_size)
{
    if (!buf || !inout_len || *inout_len < 4) return;
    size_t len = *inout_len;

    const uint8_t jfif_sig[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46};
    if (len >= sizeof(jfif_sig) && memcmp(buf, jfif_sig, sizeof(jfif_sig)) == 0) {
        return; /* 已经具备标准 JFIF 签名 */
    }

    static const uint8_t jfif_app0[] = {
        0xFF, 0xE0, 0x00, 0x10,
        0x4A, 0x46, 0x49, 0x46, 0x00, /* "JFIF\0" */
        0x01, 0x01,                   /* version 1.1 */
        0x00,                         /* density unit */
        0x00, 0x01, 0x00, 0x01,       /* Xdensity, Ydensity */
        0x00, 0x00                    /* thumbnail */
    };
    size_t app0_len = sizeof(jfif_app0);

    if (buf[0] == 0xFF && buf[1] == 0xD8 && (len + app0_len <= max_size)) {
        memmove(buf + 2 + app0_len, buf + 2, len - 2);
        memcpy(buf + 2, jfif_app0, app0_len);
        *inout_len = len + app0_len;
        ESP_LOGI(TAG, "已成功补全 LVGL 兼容 JFIF APP0 签名 (有效长度: %zu 字节)", *inout_len);
    }
}

/* URL 百分号编码 (RFC 3986)，确保特殊字符及查询参数安全传输给转码 CDN */
static void url_encode(const char *src, char *dst, size_t dst_len)
{
    if (!src || !dst || dst_len == 0) return;
    static const char hex[] = "0123456789ABCDEF";
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d + 4 < dst_len; s++) {
        unsigned char c = (unsigned char)src[s];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[d++] = (char)c;
        } else {
            dst[d++] = '%';
            dst[d++] = hex[(c >> 4) & 0x0F];
            dst[d++] = hex[c & 0x0F];
        }
    }
    dst[d] = '\0';
}

/* 从系列缩写与编号构建 Scryfall 官方高清卡图直链 (302 重定向至 cards.scryfall.io/normal/...) */
static void build_scryfall_image_url(const char *set_code, char *out_url, size_t max_len)
{
    if (!set_code || !set_code[0] || !out_url || max_len == 0) return;
    char set[16] = {0};
    char num[16] = {0};
    const char *p = set_code;
    size_t si = 0;
    while (*p && *p != ' ' && *p != '#' && si < sizeof(set) - 1) {
        set[si++] = (char)tolower((unsigned char)*p);
        p++;
    }
    while (*p && (*p == ' ' || *p == '#')) p++;
    size_t ni = 0;
    while (*p && ni < sizeof(num) - 1) {
        num[ni++] = *p++;
    }
    if (set[0] && num[0]) {
        snprintf(out_url, max_len, "https://api.scryfall.com/cards/%s/%s?format=image&version=normal", set, num);
    }
}

/* 静态 8KB 缓存用于接收英文卡牌 JSON（Scryfall 英文卡牌完整 JSON 约 5KB） */
static char s_en_json_buf[8192];

/* 请求 Scryfall 对应的英文版卡牌以补充 flavor_en 原文 */
static void fetch_english_flavor(const char *set_lower, const char *collector_number, char *out_flavor_en, size_t max_len)
{
    if (!set_lower || !collector_number || !out_flavor_en || max_len == 0) return;
    char en_url[128] = {0};
    snprintf(en_url, sizeof(en_url), "https://api.scryfall.com/cards/%s/%s/en", set_lower, collector_number);

    s_en_json_buf[0] = '\0';
    http_collector_t en_col = {
        .buffer = s_en_json_buf,
        .max_size = sizeof(s_en_json_buf) - 1,
        .length = 0,
        .is_image = false,
        .location = {0},
    };

    esp_http_client_config_t cfg = {
        .url = en_url,
        .event_handler = http_event_handler,
        .user_data = &en_col,
        .timeout_ms = 8000,
        .buffer_size = 2048,
        .buffer_size_tx = 512,
        .max_redirection_count = 3,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return;

    esp_http_client_set_header(client, "User-Agent", "FoloPassport-MTG/1.0");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && status >= 200 && status < 300 && en_col.length > 0) {
        cJSON *root = cJSON_Parse(s_en_json_buf);
        if (root) {
            cJSON *j_flv = cJSON_GetObjectItem(root, "flavor_text");
            if (!j_flv || !j_flv->valuestring) {
                /* 兼容双面牌英文风味 */
                cJSON *j_faces = cJSON_GetObjectItem(root, "card_faces");
                if (cJSON_IsArray(j_faces) && cJSON_GetArraySize(j_faces) > 0) {
                    cJSON *f0 = cJSON_GetArrayItem(j_faces, 0);
                    j_flv = cJSON_GetObjectItem(f0, "flavor_text");
                }
            }
            if (j_flv && j_flv->valuestring && j_flv->valuestring[0]) {
                snprintf(out_flavor_en, max_len, "%s", j_flv->valuestring);
                ESP_LOGI(TAG, "成功获取英文风味原文: %s", out_flavor_en);
            }
            cJSON_Delete(root);
        }
    } else {
        ESP_LOGW(TAG, "获取英文卡牌失败 (url=%s, status=%d, err=%s, len=%zu)",
                 en_url, status, esp_err_to_name(err), en_col.length);
    }
}

/* 清洗线上风味文字中包含的 \n～ 等作者署名杂音，并提取作者 */
static void clean_online_flavor(const char *in, char *out_quote, size_t quote_sz,
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

/* 解析 Scryfall 返回的卡牌 JSON 数据 */
static bool parse_scryfall_card_json(const char *json_str, mtg_card_t *out_card)
{
    if (!json_str || !out_card) return false;
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return false;

    memset(&s_online_storage, 0, sizeof(s_online_storage));

    cJSON *j_name = cJSON_GetObjectItem(root, "name");
    cJSON *j_pname = cJSON_GetObjectItem(root, "printed_name");
    cJSON *j_mana = cJSON_GetObjectItem(root, "mana_cost");
    cJSON *j_type = cJSON_GetObjectItem(root, "type_line");
    cJSON *j_ptype = cJSON_GetObjectItem(root, "printed_type_line");
    cJSON *j_otext = cJSON_GetObjectItem(root, "oracle_text");
    cJSON *j_ptext = cJSON_GetObjectItem(root, "printed_text");
    cJSON *j_flavor = cJSON_GetObjectItem(root, "flavor_text");
    cJSON *j_pow = cJSON_GetObjectItem(root, "power");
    cJSON *j_tou = cJSON_GetObjectItem(root, "toughness");
    cJSON *j_set = cJSON_GetObjectItem(root, "set");
    cJSON *j_num = cJSON_GetObjectItem(root, "collector_number");

    cJSON *j_img = NULL;
    cJSON *j_imgs = cJSON_GetObjectItem(root, "image_uris");
    if (j_imgs) {
        j_img = cJSON_GetObjectItem(j_imgs, "normal");
        if (!j_img) j_img = cJSON_GetObjectItem(j_imgs, "small");
    }

    /* 双面牌 / 多面牌支持兼容 */
    cJSON *j_faces = cJSON_GetObjectItem(root, "card_faces");
    if (cJSON_IsArray(j_faces) && cJSON_GetArraySize(j_faces) > 0) {
        cJSON *f0 = cJSON_GetArrayItem(j_faces, 0);
        if (!j_pname) j_pname = cJSON_GetObjectItem(f0, "printed_name");
        if (!j_name) j_name = cJSON_GetObjectItem(f0, "name");
        if (!j_flavor) j_flavor = cJSON_GetObjectItem(f0, "flavor_text");
        if (!j_mana) j_mana = cJSON_GetObjectItem(f0, "mana_cost");
        if (!j_ptype) j_ptype = cJSON_GetObjectItem(f0, "printed_type_line");
        if (!j_type) j_type = cJSON_GetObjectItem(f0, "type_line");
        if (!j_ptext) j_ptext = cJSON_GetObjectItem(f0, "printed_text");
        if (!j_otext) j_otext = cJSON_GetObjectItem(f0, "oracle_text");
        if (!j_img) {
            cJSON *f0_imgs = cJSON_GetObjectItem(f0, "image_uris");
            if (f0_imgs) {
                j_img = cJSON_GetObjectItem(f0_imgs, "normal");
                if (!j_img) j_img = cJSON_GetObjectItem(f0_imgs, "small");
            }
        }
    }

    /* 1. 中文牌名 */
    const char *pname_str = (j_pname && j_pname->valuestring) ? j_pname->valuestring :
                            ((j_name && j_name->valuestring) ? j_name->valuestring : "未知卡牌");
    snprintf(s_online_storage.name_zh, sizeof(s_online_storage.name_zh), "%s", pname_str);

    /* 2. 英文原名 */
    if (j_name && j_name->valuestring) {
        snprintf(s_online_storage.name_en, sizeof(s_online_storage.name_en), "%s", j_name->valuestring);
    }

    /* 3. 法术力费用 */
    if (j_mana && j_mana->valuestring) {
        snprintf(s_online_storage.mana_cost, sizeof(s_online_storage.mana_cost), "%s", j_mana->valuestring);
    }

    /* 4. 类别（优先中文译名） */
    const char *type_str = (j_ptype && j_ptype->valuestring) ? j_ptype->valuestring :
                           ((j_type && j_type->valuestring) ? j_type->valuestring : "");
    snprintf(s_online_storage.type_line, sizeof(s_online_storage.type_line), "%s", type_str);

    /* 5. 规则叙述（优先中文印刷版） */
    const char *text_str = (j_ptext && j_ptext->valuestring) ? j_ptext->valuestring :
                           ((j_otext && j_otext->valuestring) ? j_otext->valuestring : "");
    snprintf(s_online_storage.oracle_text, sizeof(s_online_storage.oracle_text), "%s", text_str);

    /* 6. 中文风味文字（清洗潜在的 \n～ 等署名并分离作者） */
    if (j_flavor && j_flavor->valuestring) {
        clean_online_flavor(j_flavor->valuestring,
                            s_online_storage.flavor_text, sizeof(s_online_storage.flavor_text),
                            s_online_storage.flavor_author, sizeof(s_online_storage.flavor_author));
    }

    /* 7. 力量 / 防御力 */
    if (j_pow && j_pow->valuestring && j_tou && j_tou->valuestring) {
        snprintf(s_online_storage.pt, sizeof(s_online_storage.pt), "%s/%s",
                 j_pow->valuestring, j_tou->valuestring);
    }

    /* 8. 系列代码与编号 */
    char set_lower[16] = {0};
    char num_str[16] = {0};
    if (j_set && j_set->valuestring && j_num && j_num->valuestring) {
        char up_set[16] = {0};
        for (size_t k = 0; j_set->valuestring[k] && k < sizeof(up_set) - 1; k++) {
            up_set[k] = (char)toupper((unsigned char)j_set->valuestring[k]);
            set_lower[k] = (char)tolower((unsigned char)j_set->valuestring[k]);
        }
        snprintf(num_str, sizeof(num_str), "%s", j_num->valuestring);
        snprintf(s_online_storage.set_code, sizeof(s_online_storage.set_code), "%s #%s",
                 up_set, j_num->valuestring);
    }

    /* 9. 牌图 URL (Scryfall small 格式) */
    if (j_img && j_img->valuestring) {
        snprintf(s_online_storage.image_url, sizeof(s_online_storage.image_url), "%s", j_img->valuestring);
    } else if (s_online_storage.set_code[0]) {
        build_scryfall_image_url(s_online_storage.set_code, s_online_storage.image_url, sizeof(s_online_storage.image_url));
    }

    cJSON_Delete(root);

    /* 10. 额外拉取原版英文风味文字 (使线上卡牌中英文对照对齐) */
    if (set_lower[0] && num_str[0]) {
        fetch_english_flavor(set_lower, num_str, s_online_storage.flavor_en, sizeof(s_online_storage.flavor_en));
    }

    out_card->name_zh = s_online_storage.name_zh;
    out_card->name_en = s_online_storage.name_en;
    out_card->mana_cost = s_online_storage.mana_cost;
    out_card->type_line = s_online_storage.type_line;
    out_card->oracle_text = s_online_storage.oracle_text;
    out_card->flavor_text = s_online_storage.flavor_text;
    out_card->flavor_en = s_online_storage.flavor_en;
    out_card->flavor_author = s_online_storage.flavor_author;
    out_card->pt = s_online_storage.pt;
    out_card->set_code = s_online_storage.set_code;
    out_card->image_url = s_online_storage.image_url;

    return true;
}

void mtg_fetch_init(void)
{
    ESP_LOGI(TAG, "MTG 在线数据抓取与牌图模块已就绪");
}

static const char * const s_scryfall_random_queries[] = {
    "https://api.scryfall.com/cards/random?q=lang:zhs+has:flavor",
    "https://api.scryfall.com/cards/random?q=lang:zhs+has:flavor+year%3C2020",
    "https://api.scryfall.com/cards/random?q=lang:zhs+has:flavor+year%3C2015",
    "https://api.scryfall.com/cards/random?q=lang:zhs+has:flavor+year%3E%3D2015+year%3C2023",
    "https://api.scryfall.com/cards/random?q=lang:zhs+has:flavor+is:booster",
    "https://api.scryfall.com/cards/random?q=lang:zhs+has:flavor+year%3C2010",
};

bool mtg_fetch_random_card(mtg_card_t *out_card, bool *is_online)
{
    if (!out_card) return false;

    bool online = mtg_wifi_prov_is_connected();
    if (online) {
        ESP_LOGI(TAG, "Wi-Fi 已连接，正在从 Scryfall 线上全时代牌库抽取随机卡牌...");

        static char s_json_buffer[12288];
        http_collector_t collector = {
            .buffer = s_json_buffer,
            .max_size = sizeof(s_json_buffer) - 1,
            .length = 0,
            .is_image = false,
        };
        s_json_buffer[0] = '\0';

        uint32_t q_idx = esp_random() % (sizeof(s_scryfall_random_queries) / sizeof(s_scryfall_random_queries[0]));
        const char *query_url = s_scryfall_random_queries[q_idx];

        esp_http_client_config_t cfg = {
            .url = query_url,
            .event_handler = http_event_handler,
            .user_data = &collector,
            .timeout_ms = 6000,
            .buffer_size = 2048,
            .buffer_size_tx = 1024,
            .max_redirection_count = 3,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        if (client) {
            esp_http_client_set_header(client, "User-Agent", "FoloPassport-MTG/1.0");
            esp_http_client_set_header(client, "Accept", "application/json");
            esp_err_t err = esp_http_client_perform(client);
            int status = esp_http_client_get_status_code(client);
            esp_http_client_cleanup(client);

            if (err == ESP_OK && status >= 200 && status < 300 && collector.length > 0) {
                if (parse_scryfall_card_json(s_json_buffer, out_card)) {
                    ESP_LOGI(TAG, "成功获取线上全量卡牌: [%s / %s]", out_card->name_zh, out_card->name_en);
                    if (is_online) *is_online = true;
                    return true;
                }
            } else {
                ESP_LOGW(TAG, "线上卡牌拉取异常 (status=%d, err=%s)，自动回退至本地 1000 库",
                         status, esp_err_to_name(err));
            }
        }
    }

    /* 离线模式或网络获取失败：从本地 1000 张真随机卡牌库抽取 */
    const mtg_card_t *fallback = mtg_cards_get_random();
    if (fallback) {
        *out_card = *fallback;
    }
    if (is_online) *is_online = false;
    return true;
}

bool mtg_fetch_card_image(const mtg_card_t *card, uint8_t *out_buf, size_t buf_size,
                          size_t *out_len, uint16_t *out_w, uint16_t *out_h)
{
    if (!card || !out_buf || buf_size == 0) return false;
    if (!mtg_wifi_prov_is_connected()) return false;

    char target_url[384] = {0};
    if (card->image_url && card->image_url[0]) {
        snprintf(target_url, sizeof(target_url), "%s", card->image_url);
    } else if (card->set_code && card->set_code[0]) {
        build_scryfall_image_url(card->set_code, target_url, sizeof(target_url));
    }

    if (!target_url[0]) {
        ESP_LOGD(TAG, "当前卡牌无可用的牌图 URL");
        return false;
    }

    /*
     * 核心技术突破说明：
     * Scryfall 官方 CDN 默认存储的所有卡牌 JPEG 均为 Progressive JPEG (0xFF, 0xC2)，
     * 而嵌入式微型单遍流式解码器 TJPGD 仅支持 Baseline JPEG (0xFF, 0xC0)。
     *
     * 策略：
     * 1. 若 target_url 已经是直接的图片 URL（例如 cards.scryfall.io），我们优先通过
     *    全球超快 CDN 转码代理 wsrv.nl 将其转换为 Baseline JPEG（输出尺寸 146x204，耗时仅 ~300ms）。
     * 2. 若直接获取的是原始流，我们在下载后严格通过 parse_jpeg_dimensions 检测 is_baseline。
     *    若检测出是 Progressive JPEG（!is_baseline），则利用获取到的直链立即通过 wsrv.nl
     *    请求标准 Baseline JPEG 流，使硬件 TJPGD 能够毫秒级硬解呈现，彻底告别白屏和解码失败！
     */
    char direct_cdn_url[384] = {0};
    char fetch_url[768] = {0};

    bool is_direct_img = (strstr(target_url, "cards.scryfall.io") != NULL) ||
                         (strstr(target_url, ".jpg") != NULL) ||
                         (strstr(target_url, ".jpeg") != NULL);

    if (is_direct_img) {
        snprintf(direct_cdn_url, sizeof(direct_cdn_url), "%s", target_url);
        char enc_url[384] = {0};
        url_encode(target_url, enc_url, sizeof(enc_url));
        snprintf(fetch_url, sizeof(fetch_url), "https://wsrv.nl/?url=%s&h=270&output=jpg&il=false", enc_url);
    } else {
        snprintf(fetch_url, sizeof(fetch_url), "%s", target_url);
    }

    ESP_LOGI(TAG, "正在获取大画幅高清牌图 (URL: %s)...", fetch_url);

    http_collector_t collector = {
        .buffer = (char *)out_buf,
        .max_size = buf_size,
        .length = 0,
        .is_image = true,
    };

    esp_http_client_config_t cfg = {
        .url = fetch_url,
        .event_handler = http_event_handler,
        .user_data = &collector,
        .timeout_ms = 12000,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
        .max_redirection_count = 4,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;

    esp_http_client_set_header(client, "User-Agent", "FoloPassport-MTG/1.0");
    esp_http_client_set_header(client, "Accept", "image/jpeg,image/*,*/*");

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    char redirect_url[384] = {0};
    if (collector.location[0]) {
        snprintf(redirect_url, sizeof(redirect_url), "%s", collector.location);
    }
    esp_http_client_cleanup(client);

    /* 若因 302 重定向拿到 Location，或者原请求未能拿到有效 JPEG 数据 */
    if ((err != ESP_OK || status < 200 || status >= 300 || collector.length < 100) && redirect_url[0]) {
        ESP_LOGI(TAG, "检测到重定向 CDN 直链: %s", redirect_url);
        snprintf(direct_cdn_url, sizeof(direct_cdn_url), "%s", redirect_url);

        char enc_redir[384] = {0};
        url_encode(redirect_url, enc_redir, sizeof(enc_redir));

        /* 采用双 CDN 容灾轮询 (主: wsrv.nl, 备: images.weserv.nl) 请求 Baseline 270px 高清流 */
        const char *cdns[] = {
            "https://wsrv.nl/?url=%s&h=270&output=jpg&il=false",
            "https://images.weserv.nl/?url=%s&h=270&output=jpg&il=false"
        };

        for (size_t c_idx = 0; c_idx < 2; c_idx++) {
            snprintf(fetch_url, sizeof(fetch_url), cdns[c_idx], enc_redir);
            collector.length = 0;
            collector.buffer[0] = '\0';
            collector.location[0] = '\0';

            esp_http_client_config_t cdn_cfg = {
                .url = fetch_url,
                .event_handler = http_event_handler,
                .user_data = &collector,
                .timeout_ms = 12000,
                .buffer_size = 2048,
                .buffer_size_tx = 1024,
                .max_redirection_count = 2,
                .crt_bundle_attach = esp_crt_bundle_attach,
            };

            esp_http_client_handle_t cdn_client = esp_http_client_init(&cdn_cfg);
            if (cdn_client) {
                esp_http_client_set_header(cdn_client, "User-Agent", "FoloPassport-MTG/1.0");
                esp_http_client_set_header(cdn_client, "Accept", "image/jpeg,image/*,*/*");
                err = esp_http_client_perform(cdn_client);
                status = esp_http_client_get_status_code(cdn_client);
                esp_http_client_cleanup(cdn_client);
            }
            if (err == ESP_OK && status >= 200 && status < 300 && collector.length > 100) {
                break;
            }
        }
    } else if ((err != ESP_OK || status < 200 || status >= 300 || collector.length < 100) && is_direct_img && direct_cdn_url[0]) {
        /* 直链首选 wsrv.nl 若偶发失败，自动回退备用 CDN images.weserv.nl */
        ESP_LOGI(TAG, "主 CDN 请求异常，尝试备用 CDN images.weserv.nl (h=270)...");
        char enc_direct[384] = {0};
        url_encode(direct_cdn_url, enc_direct, sizeof(enc_direct));
        snprintf(fetch_url, sizeof(fetch_url), "https://images.weserv.nl/?url=%s&h=270&output=jpg&il=false", enc_direct);

        collector.length = 0;
        collector.buffer[0] = '\0';

        esp_http_client_config_t bkp_cfg = {
            .url = fetch_url,
            .event_handler = http_event_handler,
            .user_data = &collector,
            .timeout_ms = 12000,
            .buffer_size = 2048,
            .buffer_size_tx = 1024,
            .max_redirection_count = 2,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        esp_http_client_handle_t bkp_client = esp_http_client_init(&bkp_cfg);
        if (bkp_client) {
            esp_http_client_set_header(bkp_client, "User-Agent", "FoloPassport-MTG/1.0");
            esp_http_client_set_header(bkp_client, "Accept", "image/jpeg,image/*,*/*");
            err = esp_http_client_perform(bkp_client);
            status = esp_http_client_get_status_code(bkp_client);
            esp_http_client_cleanup(bkp_client);
        }
    }

    if (err != ESP_OK || status < 200 || status >= 300 || collector.length < 100) {
        ESP_LOGW(TAG, "牌图拉取失败: status=%d, err=%s, len=%zu",
                 status, esp_err_to_name(err), collector.length);
        return false;
    }

    uint16_t w = 0, h = 0;
    bool is_baseline = false;
    if (!parse_jpeg_dimensions(out_buf, collector.length, &w, &h, &is_baseline)) {
        ESP_LOGW(TAG, "牌图数据非有效 JPEG 或未解析出尺寸 (len=%zu)", collector.length);
        return false;
    }

    /* 若拉取到的图片是 Progressive JPEG（如直接从源站下载未经过转码），立即转码为 Baseline JPEG 保证 TJPGD 解码安全 */
    if (!is_baseline && direct_cdn_url[0]) {
        ESP_LOGI(TAG, "检测到渐进式 JPEG (SOF2)，请求转码为嵌入式标准 Baseline JPEG (h=270)...");
        char enc_prog[384] = {0};
        url_encode(direct_cdn_url, enc_prog, sizeof(enc_prog));
        snprintf(fetch_url, sizeof(fetch_url), "https://wsrv.nl/?url=%s&h=270&output=jpg&il=false", enc_prog);

        collector.length = 0;
        collector.buffer[0] = '\0';

        esp_http_client_config_t trans_cfg = {
            .url = fetch_url,
            .event_handler = http_event_handler,
            .user_data = &collector,
            .timeout_ms = 12000,
            .buffer_size = 2048,
            .buffer_size_tx = 1024,
            .max_redirection_count = 2,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        esp_http_client_handle_t trans_client = esp_http_client_init(&trans_cfg);
        if (trans_client) {
            esp_http_client_set_header(trans_client, "User-Agent", "FoloPassport-MTG/1.0");
            esp_http_client_set_header(trans_client, "Accept", "image/jpeg,image/*,*/*");
            err = esp_http_client_perform(trans_client);
            status = esp_http_client_get_status_code(trans_client);
            esp_http_client_cleanup(trans_client);

            if (err == ESP_OK && status >= 200 && status < 300 && collector.length > 100) {
                parse_jpeg_dimensions(out_buf, collector.length, &w, &h, &is_baseline);
            }
        }
    }

    if (!is_baseline) {
        ESP_LOGW(TAG, "警告: 牌图仍然为非 Baseline 格式，可能会导致解码异常");
    }

    /* 关键步骤：补齐 LVGL is_jpg 校验所需的 JFIF APP0 签名 */
    size_t final_len = collector.length;
    ensure_jfif_header(out_buf, &final_len, buf_size);

    if (out_len) *out_len = final_len;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;

    ESP_LOGI(TAG, "大画幅牌图就绪并通过 LVGL JFIF 校验: %ux%u (Baseline: %d), %zu 字节", w, h, is_baseline, final_len);
    return true;
}

void mtg_fetch_on_wifi_connected(void)
{
    uint32_t seed = esp_random();
    mtg_cards_reshuffle(seed);
    ESP_LOGI(TAG, "Wi-Fi 已连接：离线 1000 经典精选底库已根据硬件真随机熵完成重新洗牌，线上全时代牌池已就绪！");
}
