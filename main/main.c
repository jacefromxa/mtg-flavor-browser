#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "lvgl.h"
#include "mtg_wifi_prov.h"
#include "mtg_cards.h"
#include "mtg_fetch.h"
#include "mtg_state.h"
#include "mtg_ui.h"

#include "bootloader_random.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "mtg_main";

typedef enum {
    APP_EVENT_KEY = 0,
    APP_EVENT_CARD_FETCHED,
    APP_EVENT_IMAGE_READY,
    APP_EVENT_IMAGE_FAILED,
    APP_EVENT_NET_STATUS,
    APP_EVENT_BATTERY_TICK,
    APP_EVENT_PROV_SUCCESS_TIMEOUT,
} app_event_type_t;

typedef struct {
    app_event_type_t type;
    union {
        mtg_key_t key;
        struct {
            mtg_card_t card;
            bool is_online;
        } card_data;
        struct {
            const uint8_t *data;
            size_t size;
            uint16_t w;
            uint16_t h;
        } img_data;
        struct {
            mtg_wifi_prov_status_t status;
            char ssid[33];
            char ip[16];
        } net_data;
        int battery_soc;
    };
} app_event_t;

static QueueHandle_t s_app_queue = NULL;
static QueueHandle_t s_fetch_queue = NULL;
static mtg_state_t s_app_state;
static uint8_t s_img_buffer[32768];
static esp_timer_handle_t s_prov_done_timer = NULL;

static void on_prov_done_timer(void *arg)
{
    (void)arg;
    if (!s_app_queue) return;
    app_event_t ev;
    ev.type = APP_EVENT_PROV_SUCCESS_TIMEOUT;
    xQueueSend(s_app_queue, &ev, 0);
}

typedef enum {
    FETCH_CMD_RANDOM_CARD = 1,
    FETCH_CMD_IMAGE_ONLY = 2,
} fetch_cmd_type_t;

typedef struct {
    fetch_cmd_type_t cmd;
    mtg_card_t card;
} fetch_cmd_t;

/* 按键中断回调（非阻塞，只分发轻量事件） */
static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *user_data)
{
    (void)user_data;
    if (!s_app_queue) return;

    app_event_t ev;
    ev.type = APP_EVENT_KEY;

    if (button == BSP_BTN_UP && event == BSP_BTN_CLICK) {
        ev.key = MTG_KEY_UP;
        xQueueSend(s_app_queue, &ev, 0);
    } else if (button == BSP_BTN_DOWN && event == BSP_BTN_CLICK) {
        ev.key = MTG_KEY_DOWN;
        xQueueSend(s_app_queue, &ev, 0);
    } else if (button == BSP_BTN_OK) {
        if (event == BSP_BTN_CLICK) {
            ev.key = MTG_KEY_OK_CLICK;
            xQueueSend(s_app_queue, &ev, 0);
        } else if (event == BSP_BTN_LONG) {
            ev.key = MTG_KEY_OK_LONG;
            xQueueSend(s_app_queue, &ev, 0);
        }
    }
}

/* Wi-Fi 状态回调 */
static void on_wifi_prov_status(mtg_wifi_prov_status_t status, const char *ssid, const char *ip)
{
    if (!s_app_queue) return;

    app_event_t ev;
    ev.type = APP_EVENT_NET_STATUS;
    ev.net_data.status = status;
    if (ssid) {
        strncpy(ev.net_data.ssid, ssid, sizeof(ev.net_data.ssid) - 1);
        ev.net_data.ssid[sizeof(ev.net_data.ssid) - 1] = '\0';
    } else {
        ev.net_data.ssid[0] = '\0';
    }
    if (ip) {
        strncpy(ev.net_data.ip, ip, sizeof(ev.net_data.ip) - 1);
        ev.net_data.ip[sizeof(ev.net_data.ip) - 1] = '\0';
    } else {
        ev.net_data.ip[0] = '\0';
    }

    xQueueSend(s_app_queue, &ev, 0);
}

/* 异步抽牌与牌图获取任务：在后台独立执行网络 I/O，不卡顿 UI */
static void fetch_worker_task(void *param)
{
    (void)param;
    fetch_cmd_t cmd_msg;
    while (1) {
        if (xQueueReceive(s_fetch_queue, &cmd_msg, portMAX_DELAY) == pdTRUE) {
            mtg_card_t card;
            bool is_online = false;

            if (cmd_msg.cmd == FETCH_CMD_RANDOM_CARD) {
                mtg_fetch_random_card(&card, &is_online);

                /* 1. 先将卡牌文本数据推入 UI 队列并立即渲染展示（回到风味页面） */
                app_event_t ev;
                ev.type = APP_EVENT_CARD_FETCHED;
                ev.card_data.card = card;
                ev.card_data.is_online = is_online;
                xQueueSend(s_app_queue, &ev, portMAX_DELAY);
            } else {
                /* 仅下载指定卡牌的牌图 */
                card = cmd_msg.card;
            }

            /* 2. 若当前 Wi-Fi 已连接，在后台自动拉取 Scryfall 官方小尺寸牌图 (带最多3次退避重试) */
            if (mtg_wifi_prov_is_connected()) {
                size_t img_len = 0;
                uint16_t img_w = 0, img_h = 0;
                bool img_success = false;

                for (int retry = 0; retry < 3; retry++) {
                    if (retry > 0) {
                        vTaskDelay(pdMS_TO_TICKS(500 * retry));
                        ESP_LOGI(TAG, "第 %d 次自动重试拉取牌图...", retry + 1);
                    }
                    if (mtg_fetch_card_image(&card, s_img_buffer, sizeof(s_img_buffer),
                                             &img_len, &img_w, &img_h)) {
                        img_success = true;
                        break;
                    }
                }

                if (img_success) {
                    app_event_t img_ev;
                    img_ev.type = APP_EVENT_IMAGE_READY;
                    img_ev.img_data.data = s_img_buffer;
                    img_ev.img_data.size = img_len;
                    img_ev.img_data.w = img_w;
                    img_ev.img_data.h = img_h;
                    xQueueSend(s_app_queue, &img_ev, portMAX_DELAY);
                } else {
                    app_event_t img_ev;
                    img_ev.type = APP_EVENT_IMAGE_FAILED;
                    xQueueSend(s_app_queue, &img_ev, portMAX_DELAY);
                }
            }
        }
    }
}

/* 主 UI 交互任务 */
static void ui_task(void *param)
{
    (void)param;
    app_event_t ev;
    int64_t last_bat_check = 0;

    while (1) {
        /* 等待事件，最长等待 100ms 以便检查电池 */
        if (xQueueReceive(s_app_queue, &ev, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool need_redraw = false;

            switch (ev.type) {
            case APP_EVENT_KEY: {
                mtg_action_t act = mtg_state_handle_key(&s_app_state, ev.key);
                if (act == MTG_ACTION_REDRAW) {
                    need_redraw = true;
                } else if (act == MTG_ACTION_RETRY_IMAGE) {
                    /* 仅重试当前卡牌的牌图拉取 */
                    fetch_cmd_t cmd = {
                        .cmd = FETCH_CMD_IMAGE_ONLY,
                        .card = s_app_state.current_card,
                    };
                    xQueueSend(s_fetch_queue, &cmd, 0);
                    need_redraw = true;
                } else if (act == MTG_ACTION_FETCH_CARD) {
                    /* 触发后台任务抽牌并立即刷新呈现抽牌中动画 */
                    fetch_cmd_t cmd = { .cmd = FETCH_CMD_RANDOM_CARD };
                    xQueueSend(s_fetch_queue, &cmd, 0);
                    need_redraw = true;
                } else if (act == MTG_ACTION_START_PROVISION) {
                    mtg_wifi_prov_start();
                    mtg_state_set_net_state(&s_app_state, MTG_NET_PROV_ACTIVE);
                    need_redraw = true;
                } else if (act == MTG_ACTION_STOP_PROVISION) {
                    if (s_prov_done_timer) {
                        esp_timer_stop(s_prov_done_timer);
                    }
                    mtg_wifi_prov_stop();
                    if (mtg_wifi_prov_is_connected()) {
                        mtg_state_set_net_state(&s_app_state, MTG_NET_CONNECTED);
                    } else {
                        mtg_state_set_net_state(&s_app_state, MTG_NET_DISCONNECTED);
                    }
                    need_redraw = true;
                }
                break;
            }
            case APP_EVENT_CARD_FETCHED:
                mtg_state_set_card(&s_app_state, &ev.card_data.card, ev.card_data.is_online);
                if (ev.card_data.is_online || mtg_wifi_prov_is_connected()) {
                    s_app_state.img_state = MTG_CARD_IMG_LOADING;
                }
                need_redraw = true;
                break;

            case APP_EVENT_IMAGE_READY:
                mtg_state_set_image(&s_app_state, MTG_CARD_IMG_READY,
                                    ev.img_data.data, ev.img_data.size,
                                    ev.img_data.w, ev.img_data.h);
                need_redraw = true;
                break;

            case APP_EVENT_IMAGE_FAILED:
                mtg_state_set_image(&s_app_state, MTG_CARD_IMG_FAILED, NULL, 0, 0, 0);
                need_redraw = true;
                break;

            case APP_EVENT_NET_STATUS:
                if (ev.net_data.status == MTG_PROV_CONNECTED) {
                    mtg_state_set_net_state(&s_app_state, MTG_NET_CONNECTED);
                    mtg_state_set_net_info(&s_app_state, ev.net_data.ssid, ev.net_data.ip);
                    /* 联网成功：刷新 1000 张离线库与洗牌顺序，接入线上全量库 */
                    mtg_fetch_on_wifi_connected();

                    if (s_app_state.screen == MTG_SCREEN_PROVISION) {
                        /* 处于配网界面时：在屏幕上展示「Wi-Fi 已连接成功！」2 秒，消除用户疑虑，
                         * 随后定时器自动切回卡牌浏览视图 */
                        if (!s_prov_done_timer) {
                            esp_timer_create_args_t t_args = {
                                .callback = on_prov_done_timer,
                                .arg = NULL,
                                .name = "prov_done_tmr",
                            };
                            esp_timer_create(&t_args, &s_prov_done_timer);
                        }
                        if (s_prov_done_timer) {
                            esp_timer_stop(s_prov_done_timer);
                            esp_timer_start_once(s_prov_done_timer, 2000000); /* 2 秒展示成功状态 */
                        }
                    } else {
                        /* 正常开机自动重连：直接拉取当前卡牌牌图 */
                        if (s_app_state.img_state != MTG_CARD_IMG_READY) {
                            s_app_state.img_state = MTG_CARD_IMG_LOADING;
                            fetch_cmd_t cmd = {
                                .cmd = FETCH_CMD_IMAGE_ONLY,
                                .card = s_app_state.current_card,
                            };
                            xQueueSend(s_fetch_queue, &cmd, 0);
                        }
                    }
                } else if (ev.net_data.status == MTG_PROV_CONNECTING) {
                    mtg_state_set_net_state(&s_app_state, MTG_NET_CONNECTING);
                    mtg_state_set_net_info(&s_app_state, ev.net_data.ssid, NULL);
                } else if (ev.net_data.status == MTG_PROV_AP_ACTIVE) {
                    mtg_state_set_net_state(&s_app_state, MTG_NET_PROV_ACTIVE);
                    mtg_state_set_net_info(&s_app_state, ev.net_data.ssid, ev.net_data.ip);
                } else if (ev.net_data.status == MTG_PROV_DISCONNECTED) {
                    mtg_state_set_net_state(&s_app_state, MTG_NET_DISCONNECTED);
                }
                need_redraw = true;
                break;

            case APP_EVENT_PROV_SUCCESS_TIMEOUT:
                if (s_app_state.screen == MTG_SCREEN_PROVISION) {
                    s_app_state.screen = MTG_SCREEN_BROWSER;
                    need_redraw = true;
                }
                if (s_app_state.img_state != MTG_CARD_IMG_READY) {
                    s_app_state.img_state = MTG_CARD_IMG_LOADING;
                    fetch_cmd_t cmd = {
                        .cmd = FETCH_CMD_IMAGE_ONLY,
                        .card = s_app_state.current_card,
                    };
                    xQueueSend(s_fetch_queue, &cmd, 0);
                }
                break;

            case APP_EVENT_BATTERY_TICK:
                mtg_state_set_battery(&s_app_state, ev.battery_soc);
                need_redraw = true;
                break;

            default:
                break;
            }

            if (need_redraw) {
                if (bsp_lvgl_lock(1000)) {
                    mtg_ui_refresh(&s_app_state);
                    bsp_lvgl_unlock();
                }
            }
        }

        /* 定时检查电池电量 (每 15 秒) */
        int64_t now = esp_timer_get_time();
        if (now - last_bat_check > 15000000) {
            last_bat_check = now;
            int soc = bsp_battery_soc();
            app_event_t bat_ev;
            bat_ev.type = APP_EVENT_BATTERY_TICK;
            bat_ev.battery_soc = soc;
            xQueueSend(s_app_queue, &bat_ev, 0);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== 万智牌箴言浏览器 (MTG Wisdom & Flavor Browser) 启动 ===");

    /* 1. 硬件随机数生成器 */
    bootloader_random_enable();
    uint32_t rng_seed = 0;
    esp_fill_random(&rng_seed, sizeof(rng_seed));
    bootloader_random_disable();
    mtg_cards_init(rng_seed);

    /* 2. 初始化按键驱动 */
    esp_err_t btn_err = bsp_button_init(on_button, NULL);
    if (btn_err != ESP_OK) {
        ESP_LOGE(TAG, "按键驱动初始化失败: %s", esp_err_to_name(btn_err));
    }

    /* 3. 初始化 ST7789 显示屏与背光 */
    if (bsp_display_init() != ESP_OK) {
        ESP_LOGE(TAG, "显示屏初始化失败");
        return;
    }
    bsp_display_backlight(80);

    /* 4. 初始化 LVGL 驱动与显示对象 */
    lv_display_t *disp = bsp_lvgl_init();
    if (!disp) {
        ESP_LOGE(TAG, "LVGL 初始化失败");
        return;
    }

    /* 5. 创建消息队列 */
    s_app_queue = xQueueCreate(16, sizeof(app_event_t));
    s_fetch_queue = xQueueCreate(4, sizeof(fetch_cmd_t));

    /* 6. 初始化状态机与 UI 界面 */
    mtg_state_init(&s_app_state);

    if (bsp_lvgl_lock(1000)) {
        if (!mtg_ui_create()) {
            ESP_LOGE(TAG, "MTG UI 界面创建失败");
        } else {
            mtg_ui_refresh(&s_app_state);
        }
        bsp_lvgl_unlock();
    }

    /* 7. 初始化 I2C 与 CW2017 电池计 */
    bsp_i2c_init();
    esp_err_t bat_err = bsp_battery_init();
    if (bat_err == ESP_OK) {
        int soc = bsp_battery_soc();
        mtg_state_set_battery(&s_app_state, soc);
    } else {
        ESP_LOGW(TAG, "电池芯片不可用: %s", esp_err_to_name(bat_err));
    }

    /* 8. 初始化网络与 Wi-Fi 网页热点配网系统 */
    mtg_fetch_init();
    esp_err_t wifi_err = mtg_wifi_prov_init(on_wifi_prov_status);
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi 配网系统初始化降级: %s", esp_err_to_name(wifi_err));
    }

    /* 9. 创建主处理任务与后台抽牌任务 */
    /* ui_task 需承载 LVGL 渲染树计算及 TJPGD 图片解码头部探测，分配 10KB 栈防止溢出 */
    xTaskCreate(fetch_worker_task, "mtg_fetch", 6144, NULL, 3, NULL);
    xTaskCreate(ui_task, "mtg_ui", 10240, NULL, 4, NULL);

    ESP_LOGI(TAG, "万智牌箴言浏览器就绪！剩余可用内存: %u 字节",
             (unsigned)esp_get_free_heap_size());
}
