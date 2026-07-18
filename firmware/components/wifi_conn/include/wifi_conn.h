// wifi_conn — STA接続（M5）。RTCメモリに BSSID/channel を保存し高速再接続。
// SSID/パスワードは menuconfig（CONFIG_WIFI_SSID / CONFIG_WIFI_PASSWORD）。
#pragma once
#include "esp_err.h"

// Wi-Fi STA 接続（IP取得まで）。前回 BSSID があれば高速再接続を試みる。
esp_err_t wifi_conn_connect(void);

// 切断・ドライバ停止（deep sleep 前）。
void wifi_conn_stop(void);
