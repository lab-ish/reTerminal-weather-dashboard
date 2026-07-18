// reTerminal E1002 天気ダッシュボード — 起床フロー骨格（M0）
// 各コンポーネントは M4〜M7 で本実装。M0 はスタブを順に呼び、
// アーキテクチャ（コンポーネント分割とリンク）が成立することを確認する。
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
    ESP_LOGI(TAG, "reTerminal weather dashboard (M0 scaffold, %dx%d)",
             EPD_WIDTH, EPD_HEIGHT);

    // 起床フロー（M0 はスタブ呼び出しのみ。実装は各マイルストーンで）
    int pct = battery_read_percent();          // M6: バッテリ測定
    wifi_conn_connect();                       // M5: Wi-Fi 高速再接続
    time_sync_now();                           // M7: 時刻同期・次回起床算出
    http_fetch_dashboard();                    // M5: manifest + bin GET
    framebuffer_alloc();                       // M5: PSRAM フレームバッファ
    battery_overlay_draw(pct);                 // M6: 予約矩形に % + アイコン
    epd_init();                                // M4: パネル初期化
    epd_refresh();                             // M4: 全画面フルリフレッシュ
    power_mgr_deep_sleep();                    // M7: deep sleep

    ESP_LOGI(TAG, "M0 scaffold done");
}
