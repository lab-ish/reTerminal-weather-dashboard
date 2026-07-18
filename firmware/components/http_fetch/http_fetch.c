// http_fetch 実装（M5）: HTTPS で manifest.json / dashboard.bin を取得。
#include "http_fetch.h"

#include <stdlib.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include "app_config.h"

static const char *TAG = "http_fetch";

// open→read でボディを dst へストリーム取得する共通関数。
static esp_err_t http_get(const char *url, uint8_t *dst, size_t cap, size_t *out_len)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
        .keep_alive_enable = true,
    };
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) return ESP_FAIL;

    esp_err_t err = esp_http_client_open(c, 0);
    if (err != ESP_OK) { esp_http_client_cleanup(c); return err; }

    int64_t clen = esp_http_client_fetch_headers(c);
    int status = esp_http_client_get_status_code(c);
    if (status != 200) {
        ESP_LOGE(TAG, "%s -> HTTP %d", url, status);
        esp_http_client_close(c);
        esp_http_client_cleanup(c);
        return ESP_FAIL;
    }
    if (clen > 0 && (size_t)clen > cap) {
        ESP_LOGE(TAG, "body %lld > cap %u", (long long)clen, (unsigned)cap);
        esp_http_client_close(c);
        esp_http_client_cleanup(c);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t total = 0;
    while (1) {
        int r = esp_http_client_read(c, (char *)dst + total, cap - total);
        if (r < 0) { err = ESP_FAIL; break; }
        if (r == 0) break;      // 完了
        total += r;
        if (total >= cap) break;
    }
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    if (err != ESP_OK) return err;
    if (out_len) *out_len = total;
    ESP_LOGI(TAG, "%s -> %u bytes", url, (unsigned)total);
    return ESP_OK;
}

// 自前フォーマットの小さな manifest 用の軽量パーサ（cJSON 非依存）。
// キー "key": の直後の整数を返す。scope 以降のみ探索する。
static long json_int(const char *scope, const char *key, long def)
{
    char pat[48];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(scope, pat);
    if (!p) return def;
    p = strchr(p + strlen(pat), ':');
    if (!p) return def;
    return strtol(p + 1, NULL, 10);
}

// "key":"value" の value を out へコピー。
static void json_str(const char *scope, const char *key, char *out, size_t cap)
{
    out[0] = '\0';
    char pat[48];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(scope, pat);
    if (!p) return;
    p = strchr(p + strlen(pat), ':');
    if (!p) return;
    p = strchr(p, '"');
    if (!p) return;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < cap - 1) out[i++] = *p++;
    out[i] = '\0';
}

esp_err_t http_fetch_manifest(dashboard_meta_t *meta)
{
    if (!meta) return ESP_ERR_INVALID_ARG;
    char buf[1024];
    size_t len = 0;
    esp_err_t err = http_get(URL_MANIFEST, (uint8_t *)buf, sizeof(buf) - 1, &len);
    if (err != ESP_OK) return err;
    buf[len] = '\0';

    memset(meta, 0, sizeof(*meta));
    meta->width = (int)json_int(buf, "width", EPD_WIDTH);
    meta->height = (int)json_int(buf, "height", EPD_HEIGHT);
    meta->bytes = (size_t)json_int(buf, "bytes", EPD_FB_BYTES);
    json_str(buf, "format", meta->format, sizeof(meta->format));
    json_str(buf, "content_hash", meta->content_hash, sizeof(meta->content_hash));

    // battery_rect の x/y/w/h は "battery_rect" 以降を scope にして取得
    const char *br = strstr(buf, "\"battery_rect\"");
    const char *s = br ? br : buf;
    meta->batt_x = (int)json_int(s, "x", BATTERY_RECT_X);
    meta->batt_y = (int)json_int(s, "y", BATTERY_RECT_Y);
    meta->batt_w = (int)json_int(s, "w", BATTERY_RECT_W);
    meta->batt_h = (int)json_int(s, "h", BATTERY_RECT_H);
    return ESP_OK;
}

esp_err_t http_fetch_bin(uint8_t *dst, size_t cap, size_t *out_len)
{
    if (!dst) return ESP_ERR_INVALID_ARG;
    return http_get(URL_BIN, dst, cap, out_len);
}
