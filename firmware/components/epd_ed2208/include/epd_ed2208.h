// epd_ed2208 — M4 で実装（M0 はスタブ）
#pragma once
#include "esp_err.h"
esp_err_t epd_init(void);
esp_err_t epd_refresh(void);
