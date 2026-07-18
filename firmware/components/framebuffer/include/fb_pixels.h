// fb_pixels — 4bppフレームバッファの純ピクセル操作（ESP-IDF非依存, host テスト可）。
// レイアウト: 1バイト=2px, 上位ニブル=左(偶数x), 下位ニブル=右(奇数x)。
// byte index = (y*EPD_WIDTH + x)/2。ニブル値は palette（黒0/白1/黄2/赤3/青5/緑6）。
#pragma once
#include <stddef.h>
#include <stdint.h>

void fb_set_nibble(uint8_t *buf, int x, int y, uint8_t nib);
uint8_t fb_get_nibble(const uint8_t *buf, int x, int y);
void fb_fill(uint8_t *buf, uint8_t nib);
void fb_fill_rect(uint8_t *buf, int x, int y, int w, int h, uint8_t nib);

// M8 実機の色/インデックス確認用: 6色の縦帯（黒/白/黄/赤/青/緑）。
void fb_test_pattern(uint8_t *buf);
