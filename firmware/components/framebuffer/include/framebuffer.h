// framebuffer — PSRAM 上の 4bpp フレームバッファ管理（M5）。
// 純ピクセル操作は fb_pixels.h（host テスト可）。
#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "fb_pixels.h"

// PSRAM に EPD_FB_BYTES(192000B) を確保。既に確保済みなら再利用。
esp_err_t framebuffer_alloc(void);

// 確保済みバッファ（未確保なら NULL）。
uint8_t *framebuffer_get(void);

void framebuffer_free(void);
