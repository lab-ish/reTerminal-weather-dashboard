// http_fetch — GitHub Pages から manifest.json / dashboard.bin を HTTPS 取得（M5）。
// esp_crt_bundle で github.io を検証。
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int width;
    int height;
    size_t bytes;                 // dashboard.bin のバイト数（=192000 期待）
    int batt_x, batt_y, batt_w, batt_h;  // battery_rect（overlay 用）
    char content_hash[65];        // sha256 hex（初版未使用・将来のスキップ判定用）
    char format[24];              // "ed2208_4bpp"
} dashboard_meta_t;

// manifest.json を取得しパースする。
esp_err_t http_fetch_manifest(dashboard_meta_t *meta);

// dashboard.bin を dst（容量 cap）へストリーム取得。実バイト数を out_len に返す。
esp_err_t http_fetch_bin(uint8_t *dst, size_t cap, size_t *out_len);
