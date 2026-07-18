// time_sync スタブ（M7 で本実装）
#include "time_sync.h"
#include "esp_log.h"
static const char *TAG = "time_sync";
esp_err_t time_sync_now(void){ ESP_LOGI(TAG,"stub"); return ESP_OK; }
