// battery — バッテリ電圧 ADC 測定（M6）。GPIO21=H で有効化 → GPIO1 ADC → ×2.0。
#pragma once
#include "esp_err.h"
#include "battery_calc.h"

// 残量% と 5段階セグメント数を測定して返す（どちらも NULL 可）。
esp_err_t battery_measure(int *out_percent, int *out_bucket);

// 便宜: 残量% のみ（失敗時 -1）。
int battery_read_percent(void);
