// battery_overlay — 予約矩形に 残量% + 5段階アイコンをニブル直書き（M6）。
// framebuffer 上の純描画（ESP-IDF非依存, host テスト可）。
#pragma once
#include <stdint.h>

// buf の予約矩形 (rx,ry,rw,rh) に "NN%" と bucket 段（0..levels）の
// バッテリアイコンを描画する。矩形は白で塗り直してから重ね書きする。
void battery_overlay_draw(uint8_t *buf, int rx, int ry, int rw, int rh,
                          int percent, int bucket, int levels);
