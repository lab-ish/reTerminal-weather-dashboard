#include "battery_calc.h"

// LiPo 放電曲線（端子電圧→残量%）。実測で M8 に補正する。
static const struct { float v; int pct; } kCurve[] = {
    {4.20f, 100}, {4.10f, 90}, {4.00f, 80}, {3.90f, 65}, {3.80f, 50},
    {3.70f, 30}, {3.60f, 15}, {3.50f, 7}, {3.40f, 2}, {3.30f, 0},
};
static const int kN = sizeof(kCurve) / sizeof(kCurve[0]);

int batt_voltage_to_percent(float volts)
{
    if (volts >= kCurve[0].v) return 100;
    if (volts <= kCurve[kN - 1].v) return 0;
    for (int i = 0; i < kN - 1; i++) {
        float hi = kCurve[i].v, lo = kCurve[i + 1].v;
        if (volts <= hi && volts >= lo) {
            float t = (volts - lo) / (hi - lo);           // 0..1
            float pct = kCurve[i + 1].pct + t * (kCurve[i].pct - kCurve[i + 1].pct);
            int p = (int)(pct + 0.5f);
            return p < 0 ? 0 : (p > 100 ? 100 : p);
        }
    }
    return 0;
}

int batt_percent_to_bucket(int percent, int levels)
{
    if (levels <= 0) return 0;
    if (percent <= 0) return 0;
    if (percent >= 100) return levels;
    // 1..levels に均等割り（percent>0 は最低1セグメント点灯）
    int b = (percent * levels + 99) / 100;   // ceil
    if (b < 1) b = 1;
    if (b > levels) b = levels;
    return b;
}
