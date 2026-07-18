// power_calc — 次回起床までの秒数計算（ESP-IDF非依存, host テスト可）。
#pragma once

// now_utc_epoch: 現在のUTC epoch秒。hours_jst[]: 起床する JST の時刻(0-23) n個。
// JST基準で直近“未来”の該当時刻までの秒数を返す（同時刻ちょうどなら次の周期=+24h）。
long seconds_to_next_wake(long now_utc_epoch, const int *hours_jst, int n);
