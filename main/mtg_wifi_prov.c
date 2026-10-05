#include "mtg_wifi_prov.h"
#include "mtg_radio.h"

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "mtg_wifi_prov";

#define SOFTAP_SSID "FoloPassport-WiFi"
#define NVS_NAMESPACE "mtg_wifi"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASS "pass"

static mtg_wifi_prov_cb_t s_cb = NULL;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;
static httpd_handle_t s_httpd = NULL;
static TaskHandle_t s_dns_task_handle = NULL;
static int s_dns_sock = -1;
static esp_timer_handle_t s_stop_ap_timer = NULL;

static char s_cur_ssid[33] = {0};
static char s_cur_ip[16] = {0};
static bool s_is_connected = false;
static bool s_is_ap_active = false;
static bool s_wifi_inited = false;

static void on_stop_ap_timer(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "延迟定时器触发：正在安全注销 SoftAP 与配网服务 (彻底杜绝事件回调死锁)...");
    mtg_wifi_prov_stop();
}

/* URL 解码辅助函数 */
static void url_decode(char *dst, const char *src, size_t max_len)
{
    char a, b;
    size_t d_idx = 0;
    while (*src && d_idx + 1 < max_len) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit((int)a) && isxdigit((int)b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            dst[d_idx++] = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            dst[d_idx++] = ' ';
            src++;
        } else {
            dst[d_idx++] = *src++;
        }
    }
    dst[d_idx] = '\0';
}

/* 极简 DNS 劫持服务任务（实现 iOS/Android 强制门户 Captive Portal 自动弹窗） */
static void dns_server_task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t rx_buf[256];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    ESP_LOGI(TAG, "DNS 劫持服务器已启动 (端口 53)");

    /* 设置 500ms 超时，确保任务在 AP 注销时能平滑退出循环，杜绝跨线程 close 阻塞或崩溃 */
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 500000,
    };
    if (s_dns_sock >= 0) {
        setsockopt(s_dns_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    while (s_is_ap_active && s_dns_sock >= 0) {
        int len = recvfrom(s_dns_sock, rx_buf, sizeof(rx_buf), 0,
                           (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 12) {
            continue;
        }

        /* 构造 DNS 响应：解析到 192.168.4.1 */
        rx_buf[2] = 0x81; /* Flags: 响应 + 递归可用 */
        rx_buf[3] = 0x80;
        rx_buf[6] = 0x00; /* Answers count = 1 */
        rx_buf[7] = 0x01;

        int idx = len;
        if (idx + 16 < (int)sizeof(rx_buf)) {
            rx_buf[idx++] = 0xC0; /* 指向 Query Name 的指针 */
            rx_buf[idx++] = 0x0C;
            rx_buf[idx++] = 0x00; /* Type: A (IPv4) */
            rx_buf[idx++] = 0x01;
            rx_buf[idx++] = 0x00; /* Class: IN */
            rx_buf[idx++] = 0x01;
            rx_buf[idx++] = 0x00; /* TTL: 60 秒 */
            rx_buf[idx++] = 0x00;
            rx_buf[idx++] = 0x00;
            rx_buf[idx++] = 0x3C;
            rx_buf[idx++] = 0x00; /* Data length: 4 字节 */
            rx_buf[idx++] = 0x04;
            rx_buf[idx++] = 192;  /* IP: 192.168.4.1 */
            rx_buf[idx++] = 168;
            rx_buf[idx++] = 4;
            rx_buf[idx++] = 1;

            sendto(s_dns_sock, rx_buf, idx, 0, (struct sockaddr *)&client_addr, client_addr_len);
        }
    }

    if (s_dns_sock >= 0) {
        int fd = s_dns_sock;
        s_dns_sock = -1;
        close(fd);
    }
    s_dns_task_handle = NULL;
    ESP_LOGI(TAG, "DNS 劫持服务任务已安全退出");
    vTaskDelete(NULL);
}

/* 配网页面 GET 响应：包含深色万智牌风格与扫描到的周边 Wi-Fi 列表 */
static const char s_prov_html_head[] =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>万智牌箴言浏览器 · Wi-Fi 配网</title><style>"
    "body{background:#0d1117;color:#f0e6d2;font-family:-apple-system,BlinkMacSystemFont,sans-serif;margin:0;padding:20px;display:flex;justify-content:center;}"
    ".card{background:#161b22;border:1px solid #8b6508;border-radius:12px;padding:24px;max-width:380px;width:100%;box-shadow:0 4px 16px rgba(0,0,0,0.5);}"
    "h2{color:#d4af37;margin-top:0;font-size:20px;text-align:center;}"
    "p{font-size:13px;color:#8b949e;line-height:1.5;}"
    "label{display:block;font-size:14px;color:#c9d1d9;margin-top:16px;margin-bottom:6px;}"
    "select,input[type=text],input[type=password]{width:100%;padding:10px 12px;border-radius:6px;border:1px solid #30363d;background:#0d1117;color:#f0e6d2;font-size:15px;box-sizing:border-box;}"
    "button{width:100%;margin-top:24px;padding:12px;background:#238636;border:none;border-radius:6px;color:#fff;font-size:16px;font-weight:bold;cursor:pointer;}"
    "button:active{background:#2ea043;}"
    ".tip{font-size:12px;color:#d4af37;margin-top:12px;text-align:center;}"
    "</style></head><body><div class=\"card\">"
    "<h2>万智牌箴言浏览器</h2>"
    "<p>请选择您家中的 2.4GHz Wi-Fi 并输入密码以连接网络。</p>"
    "<form action=\"/connect\" method=\"POST\">"
    "<label>选择 Wi-Fi (SSID)：</label>"
    "<select id=\"sel\" onchange=\"var c=document.getElementById('custom');if(this.value==='_custom_'){c.style.display='block';}else{c.style.display='none';}\">";

static const char s_prov_html_tail[] =
    "<option value=\"_custom_\">-- 手动输入其他 Wi-Fi --</option>"
    "</select>"
    "<input type=\"text\" id=\"custom\" placeholder=\"手动输入 Wi-Fi 名称\" style=\"display:none;margin-top:8px;\">"
    "<input type=\"hidden\" id=\"real_ssid\" name=\"ssid\">"
    "<label>Wi-Fi 密码：</label>"
    "<input type=\"password\" name=\"password\" placeholder=\"输入密码（无密码可留空）\">"
    "<button type=\"submit\" onclick=\"var s=document.getElementById('sel').value;document.getElementById('real_ssid').value=(s==='_custom_')?document.getElementById('custom').value:s;\">连接 Wi-Fi</button>"
    "</form><div class=\"tip\">提示：配网成功后设备将自动关闭热点。</div></div></body></html>";

/* 配网页面 GET 响应：包含深色万智牌风格与扫描到的周边 Wi-Fi 列表 */
static esp_err_t prov_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send_chunk(req, s_prov_html_head, HTTPD_RESP_USE_STRLEN);

    /* 扫描附近 Wi-Fi 列表 */
    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    esp_wifi_scan_start(&scan_cfg, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count > 10) ap_count = 10;

    if (ap_count > 0) {
        wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_list) {
            esp_wifi_scan_get_ap_records(&ap_count, ap_list);
            for (int i = 0; i < ap_count; i++) {
                if (strlen((char *)ap_list[i].ssid) > 0) {
                    char opt[128];
                    snprintf(opt, sizeof(opt), "<option value=\"%s\">%s (信号 %d dBm)</option>",
                             (char *)ap_list[i].ssid, (char *)ap_list[i].ssid, ap_list[i].rssi);
                    httpd_resp_send_chunk(req, opt, HTTPD_RESP_USE_STRLEN);
                }
            }
            free(ap_list);
        }
    }

    httpd_resp_send_chunk(req, s_prov_html_tail, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* 配网表单 POST 提交响应 */
static esp_err_t prov_connect_handler(httpd_req_t *req)
{
    char post_buf[256] = {0};
    int ret = httpd_req_recv(req, post_buf, sizeof(post_buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char raw_ssid[64] = {0};
    char raw_pass[64] = {0};
    char final_ssid[33] = {0};
    char final_pass[64] = {0};

    char *p_ssid = strstr(post_buf, "ssid=");
    if (p_ssid) {
        p_ssid += 5;
        char *p_amp = strchr(p_ssid, '&');
        if (p_amp) {
            size_t len = p_amp - p_ssid;
            if (len < sizeof(raw_ssid)) {
                strncpy(raw_ssid, p_ssid, len);
            }
        } else {
            strncpy(raw_ssid, p_ssid, sizeof(raw_ssid) - 1);
        }
    }

    char *p_pass = strstr(post_buf, "password=");
    if (p_pass) {
        p_pass += 9;
        char *p_amp = strchr(p_pass, '&');
        if (p_amp) {
            size_t len = p_amp - p_pass;
            if (len < sizeof(raw_pass)) {
                strncpy(raw_pass, p_pass, len);
            }
        } else {
            strncpy(raw_pass, p_pass, sizeof(raw_pass) - 1);
        }
    }

    url_decode(final_ssid, raw_ssid, sizeof(final_ssid));
    url_decode(final_pass, raw_pass, sizeof(final_pass));

    ESP_LOGI(TAG, "收到 Web 配网配置: SSID='%s'", final_ssid);

    const char *resp = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
                       "<title>配网中</title><style>body{background:#0d1117;color:#f0e6d2;font-family:sans-serif;padding:30px;text-align:center;}"
                       ".c{background:#161b22;border:1px solid #8b6508;border-radius:12px;padding:24px;max-width:360px;margin:auto;}"
                       "h3{color:#7ee787;}</style></head><body><div class=\"c\">"
                       "<h3>已接收 Wi-Fi 配置！</h3><p>设备正在连接路由器，请查看设备屏幕。</p>"
                       "</div></body></html>";
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    /* 保存配置到 NVS */
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_SSID, final_ssid);
        nvs_set_str(h, NVS_KEY_PASS, final_pass);
        nvs_commit(h);
        nvs_close(h);
    }

    /* 启动 STA 尝试连接 */
    strncpy(s_cur_ssid, final_ssid, sizeof(s_cur_ssid) - 1);
    wifi_config_t sta_cfg = {0};
    strncpy((char *)sta_cfg.sta.ssid, final_ssid, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, final_pass, sizeof(sta_cfg.sta.password));
    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_connect();

    if (s_cb) {
        s_cb(MTG_PROV_CONNECTING, final_ssid, NULL);
    }

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            if (s_cur_ssid[0]) {
                esp_wifi_connect();
            }
        } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI(TAG, "Wi-Fi STA 已连上 AP，等待获取 IP...");
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "Wi-Fi STA 连接断开");
            s_is_connected = false;
            s_cur_ip[0] = '\0';
            if (s_cb) {
                s_cb(MTG_PROV_DISCONNECTED, s_cur_ssid, NULL);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        esp_ip4addr_ntoa(&ev->ip_info.ip, s_cur_ip, sizeof(s_cur_ip));
        s_is_connected = true;
        ESP_LOGI(TAG, "Wi-Fi 获取到 IP 地址: %s", s_cur_ip);

        if (s_cb) {
            s_cb(MTG_PROV_CONNECTED, s_cur_ssid, s_cur_ip);
        }

        /* 成功联网后，启动 3 秒单次定时器在独立任务中异步关闭 SoftAP，
         * 绝不直接在事件回调内执行阻塞式 esp_wifi_set_mode / httpd_stop，彻底消除系统死锁！ */
        if (s_is_ap_active) {
            if (!s_stop_ap_timer) {
                esp_timer_create_args_t timer_args = {
                    .callback = on_stop_ap_timer,
                    .arg = NULL,
                    .name = "stop_ap_tmr",
                };
                esp_timer_create(&timer_args, &s_stop_ap_timer);
            }
            if (s_stop_ap_timer) {
                esp_timer_stop(s_stop_ap_timer);
                esp_timer_start_once(s_stop_ap_timer, 3000000); /* 3 秒后安全注销 SoftAP */
            }
        }
    }
}

esp_err_t mtg_wifi_prov_init(mtg_wifi_prov_cb_t cb)
{
    s_cb = cb;

    esp_err_t err = mtg_radio_nvs_prepare();
    if (err != ESP_OK) return err;
    err = mtg_radio_network_prepare();
    if (err != ESP_OK) return err;

    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }
    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }

    if (!s_wifi_inited) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        err = esp_wifi_init(&cfg);
        if (err != ESP_OK) return err;
        s_wifi_inited = true;

        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                            wifi_event_handler, NULL, NULL);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                            wifi_event_handler, NULL, NULL);
    }

    /* 检查 NVS 中是否有保存的 Wi-Fi 凭证 */
    char saved_ssid[33] = {0};
    char saved_pass[64] = {0};
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        size_t s_len = sizeof(saved_ssid);
        size_t p_len = sizeof(saved_pass);
        if (nvs_get_str(h, NVS_KEY_SSID, saved_ssid, &s_len) == ESP_OK) {
            nvs_get_str(h, NVS_KEY_PASS, saved_pass, &p_len);
        }
        nvs_close(h);
    }

    if (saved_ssid[0]) {
        ESP_LOGI(TAG, "从 NVS 读出历史 Wi-Fi: '%s'，尝试自动连接...", saved_ssid);
        strncpy(s_cur_ssid, saved_ssid, sizeof(s_cur_ssid) - 1);

        esp_wifi_set_mode(WIFI_MODE_STA);
        wifi_config_t sta_cfg = {0};
        strncpy((char *)sta_cfg.sta.ssid, saved_ssid, sizeof(sta_cfg.sta.ssid));
        strncpy((char *)sta_cfg.sta.password, saved_pass, sizeof(sta_cfg.sta.password));
        esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
        esp_wifi_start();

        if (s_cb) {
            s_cb(MTG_PROV_CONNECTING, saved_ssid, NULL);
        }
    } else {
        ESP_LOGI(TAG, "NVS 无历史 Wi-Fi 凭证，处于离线模式");
    }

    return ESP_OK;
}

esp_err_t mtg_wifi_prov_start(void)
{
    if (s_is_ap_active) return ESP_OK;

    ESP_LOGI(TAG, "启动 SoftAP 网页配网模式: SSID='%s'", SOFTAP_SSID);

    /* 切换到 APSTA 模式 */
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = SOFTAP_SSID,
            .ssid_len = strlen(SOFTAP_SSID),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();

    /* 启动 HTTP 服务器 */
    if (!s_httpd) {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.stack_size = 4096;
        config.max_open_sockets = 4;
        config.lru_purge_enable = true;

        if (httpd_start(&s_httpd, &config) == ESP_OK) {
            httpd_uri_t get_uri = {
                .uri = "/",
                .method = HTTP_GET,
                .handler = prov_get_handler,
                .user_ctx = NULL,
            };
            httpd_register_uri_handler(s_httpd, &get_uri);

            httpd_uri_t post_uri = {
                .uri = "/connect",
                .method = HTTP_POST,
                .handler = prov_connect_handler,
                .user_ctx = NULL,
            };
            httpd_register_uri_handler(s_httpd, &post_uri);

            /* 注册 Captive Portal 常见重定向路径 */
            httpd_uri_t get_hotspot = {
                .uri = "/hotspot-detect.html",
                .method = HTTP_GET,
                .handler = prov_get_handler,
                .user_ctx = NULL,
            };
            httpd_register_uri_handler(s_httpd, &get_hotspot);

            httpd_uri_t get_gen204 = {
                .uri = "/generate_204",
                .method = HTTP_GET,
                .handler = prov_get_handler,
                .user_ctx = NULL,
            };
            httpd_register_uri_handler(s_httpd, &get_gen204);
        }
    }

    /* 启动 UDP 53 DNS 劫持服务 */
    if (s_dns_sock < 0) {
        s_dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (s_dns_sock >= 0) {
            struct sockaddr_in serv_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(53),
                .sin_addr.s_addr = htonl(INADDR_ANY),
            };
            if (bind(s_dns_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
                s_is_ap_active = true;
                xTaskCreate(dns_server_task, "dns_server", 3072, NULL, 3, &s_dns_task_handle);
            } else {
                close(s_dns_sock);
                s_dns_sock = -1;
            }
        }
    }

    s_is_ap_active = true;

    if (s_cb) {
        s_cb(MTG_PROV_AP_ACTIVE, SOFTAP_SSID, "192.168.4.1");
    }

    return ESP_OK;
}

esp_err_t mtg_wifi_prov_stop(void)
{
    if (!s_is_ap_active) return ESP_OK;

    ESP_LOGI(TAG, "停止 SoftAP 网页配网模式 (注销 HTTP 与 DNS 服务)...");
    s_is_ap_active = false;

    if (s_stop_ap_timer) {
        esp_timer_stop(s_stop_ap_timer);
    }

    /* 等待 DNS 任务超时自退出并释放 socket */
    int wait_count = 0;
    while (s_dns_task_handle != NULL && wait_count++ < 15) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* 关闭 HTTP 服务器 */
    if (s_httpd) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }

    /* 切换回纯 STA 模式 */
    esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_LOGI(TAG, "SoftAP 网页配网服务已彻底注销，当前处于纯 STA 联网模式");

    return ESP_OK;
}

bool mtg_wifi_prov_is_connected(void)
{
    return s_is_connected;
}

const char *mtg_wifi_prov_get_ssid(void)
{
    return s_cur_ssid;
}

const char *mtg_wifi_prov_get_ip(void)
{
    return s_cur_ip;
}
