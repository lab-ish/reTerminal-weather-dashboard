// time_sync 実装（M7）: SNTP 同期 + JST 設定。
#include "time_sync.h"

#include <time.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

static const char *TAG = "time_sync";

esp_err_t time_sync_now(void)
{
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_err_t err = esp_netif_sntp_init(&cfg);
    if (err != ESP_OK) return err;

    // 最大10秒待つ
    err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
    esp_netif_sntp_deinit();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SNTP 同期タイムアウト: %s", esp_err_to_name(err));
        return err;
    }

    // JST 設定（TZ）。power_calc は UTC epoch を使うため実質は表示・ログ用。
    setenv("TZ", "JST-9", 1);
    tzset();

    time_t now = time(NULL);
    struct tm tm_jst;
    localtime_r(&now, &tm_jst);
    ESP_LOGI(TAG, "time synced: %04d-%02d-%02d %02d:%02d JST",
             tm_jst.tm_year + 1900, tm_jst.tm_mon + 1, tm_jst.tm_mday,
             tm_jst.tm_hour, tm_jst.tm_min);
    return ESP_OK;
}
