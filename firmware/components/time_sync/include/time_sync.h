// time_sync — SNTP 時刻同期 + JST タイムゾーン設定（M7）。
// Wi-Fi 接続後に呼ぶ。毎起床で再同期し deep sleep タイマのドリフトを自己補正する。
#pragma once
#include "esp_err.h"

esp_err_t time_sync_now(void);
