// reTerminal E1002 天気ダッシュボード — 起床フロー（M4〜M7）
// deep sleep から起床 → バッテリ測定 → Wi-Fi → 時刻同期 → 画像取得 →
// 予約矩形にバッテリ重ね書き → E Ink フルリフレッシュ → deep sleep。
// ※ 実機検証は M8（オーナーと共同）。本Macは実機未接続。
#include "esp_log.h"

#include "app_config.h"
#include "battery.h"
#include "battery_overlay.h"
#include "epd_ed2208.h"
#include "framebuffer.h"
#include "http_fetch.h"
#include "power_mgr.h"
#include "time_sync.h"
#include "wifi_conn.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "wake: reTerminal weather (%dx%d)", EPD_WIDTH, EPD_HEIGHT);

    // 1) バッテリ測定（GPIO21=H → ADC → % / 5段階 → GPIO21=L）
    int pct = 0, bucket = 0;
    if (battery_measure(&pct, &bucket) != ESP_OK) {
        ESP_LOGW(TAG, "battery 測定失敗");
    }

    // 2) Wi-Fi 高速再接続
    if (wifi_conn_connect() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi 接続失敗 → 表示更新スキップして sleep");
        power_mgr_deep_sleep();
        return;
    }

    // 3) 時刻同期（deep sleep タイマのドリフト自己補正）
    time_sync_now();

    // 4) manifest → 5) フレームバッファへ dashboard.bin
    dashboard_meta_t meta = {0};
    esp_err_t err = http_fetch_manifest(&meta);
    bool have_image = false;
    if (err == ESP_OK && framebuffer_alloc() == ESP_OK) {
        size_t got = 0;
        if (http_fetch_bin(framebuffer_get(), EPD_FB_BYTES, &got) == ESP_OK &&
            got == (size_t)EPD_FB_BYTES) {
            have_image = true;
        } else {
            ESP_LOGE(TAG, "dashboard.bin 取得失敗 (got=%u)", (unsigned)got);
        }
    } else {
        ESP_LOGE(TAG, "manifest 取得失敗");
    }

    // 6) 予約矩形にバッテリ % + 5段階アイコンを重ね書き
    if (have_image) {
        battery_overlay_draw(framebuffer_get(), meta.batt_x, meta.batt_y,
                             meta.batt_w, meta.batt_h, pct, bucket, BATT_LEVELS);

        // 7) E Ink 全画面フルリフレッシュ
        if (epd_init() == ESP_OK) {
            epd_write_framebuffer(framebuffer_get(), EPD_FB_BYTES);
            epd_refresh();
            epd_sleep();
        }
    }

    // 8) 後始末 → deep sleep
    wifi_conn_stop();
    power_mgr_deep_sleep();   // 戻らない
}
