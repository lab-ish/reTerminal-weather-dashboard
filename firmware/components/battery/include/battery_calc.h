// battery_calc — バッテリ電圧→残量%→段数 の純ロジック（ESP-IDF非依存, host テスト可）。
#pragma once

// LiPo 端子電圧[V]（3.3〜4.2）を残量%（0〜100）へ。区分線形の放電曲線。
int batt_voltage_to_percent(float volts);

// 残量%（0〜100）を 5段階の点灯セグメント数（0〜levels）へ。
int batt_percent_to_bucket(int percent, int levels);
