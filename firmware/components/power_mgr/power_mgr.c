// power_mgr 実装（M7）: 8時刻の直近未来へタイマ起床 + GPIO4 EXT1 手動起床。
#include "power_mgr.h"

#include <time.h>

#include "esp_log.h"
#include "esp_sleep.h"

#include "app_config.h"

static const char *TAG = "power_mgr";

void power_mgr_deep_sleep(void)
{
    const int hours[] = UPDATE_HOURS_JST;
    const int n = sizeof(hours) / sizeof(hours[0]);

    long now = (long)time(NULL);
    long secs = seconds_to_next_wake(now, hours, n);
    if (secs < 30) secs = 30;      // 最低30s（時刻未同期などの保険）

    ESP_LOGI(TAG, "deep sleep %ld s (next wake)", secs);

    // タイマ起床
    esp_sleep_enable_timer_wakeup((uint64_t)secs * 1000000ULL);
    // GPIO4(右ボタン, アクティブLow)で手動即時更新（EXT1, Low レベル起床）
    esp_sleep_enable_ext1_wakeup_io((1ULL << PIN_BTN2), ESP_EXT1_WAKEUP_ANY_LOW);

    esp_deep_sleep_start();          // 戻らない
}
