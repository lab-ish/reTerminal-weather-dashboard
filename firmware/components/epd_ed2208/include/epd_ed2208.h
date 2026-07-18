// epd_ed2208 — reTerminal E1002 の 7.3" 6色 E Ink（GDEP073E01 / ED2208）ドライバ。
// Seeed_GxEPD2 GxEPD2_730c_GDEP073E01 の初期化/転送/リフレッシュ手順を ESP-IDF へ移植。
//
// 重要: クラウドが ED2208 ネイティブ 4bpp（黒0/白1/黄2/赤3/青5/緑6, 1B=2px, 上位=左）で
// 配信するため、デバイス側は色変換せず framebuffer をそのまま cmd 0x10 へ流す。
// 全画面フルリフレッシュのみ（Spectra6 は部分更新不可）。実測 25〜30s。
//
// ※ 初期化バイト列・SPIクロック・busyタイムアウトは Seeed_GxEPD2 準拠。実機（M8）で確定。
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

// SPI初期化 + パネル初期化 + 電源ON。起床ごとに1回呼ぶ。
esp_err_t epd_init(void);

// フレームバッファ（EPD_FB_BYTES = 192000B, 4bppネイティブ）をパネルRAMへ転送。
esp_err_t epd_write_framebuffer(const uint8_t *fb, size_t len);

// 全画面フルリフレッシュ（DRF）。完了まで最大 full_refresh_time 待つ。
esp_err_t epd_refresh(void);

// パネルをディープスリープ（hibernate 0x07/0xA5）+ 電源OFF。deep sleep 前に呼ぶ。
esp_err_t epd_sleep(void);
