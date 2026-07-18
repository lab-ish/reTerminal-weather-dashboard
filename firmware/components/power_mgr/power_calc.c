#include "power_calc.h"

#define DAY 86400L
#define JST_OFFSET (9L * 3600L)

long seconds_to_next_wake(long now_utc_epoch, const int *hours_jst, int n)
{
    if (n <= 0) return DAY;
    long jst = now_utc_epoch + JST_OFFSET;
    long sod = ((jst % DAY) + DAY) % DAY;   // JST の 0時からの経過秒（負対策）
    long best = DAY;                         // 上限=24h
    for (int i = 0; i < n; i++) {
        int h = hours_jst[i];
        if (h < 0 || h > 23) continue;
        long target = (long)h * 3600L;
        long delta = target - sod;
        if (delta <= 0) delta += DAY;        // 過ぎている/同時刻ちょうど → 翌周期
        if (delta < best) best = delta;
    }
    return best;
}
