// power_mgr — deep sleep 電源管理（M7）。8時刻の直近未来へタイマ起床、
// GPIO4(右ボタン)を EXT1 ウェイクに割り当て手動即時更新も可能にする。
#pragma once
#include "power_calc.h"

// 次回起床時刻を算出して deep sleep へ入る（戻らない）。
void power_mgr_deep_sleep(void);
