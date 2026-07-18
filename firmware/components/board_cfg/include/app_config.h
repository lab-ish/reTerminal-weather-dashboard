// アプリ全体のコンパイル時定数。
// レイアウト系（battery_rect・パレット）は shared/layout_contract.md と一致させる
// フォールバック値。実行時は manifest.json の値を優先する。
// ピン配置は Seeed 公式 wiki（GDEP073E01 / ED2208）由来。M8 実機で最終確認する。
#pragma once

// ---- パネル ----------------------------------------------------------------
#define EPD_WIDTH   800
#define EPD_HEIGHT  480
#define EPD_FB_BYTES (EPD_WIDTH * EPD_HEIGHT / 2)   // 4bpp = 192000 B

// e-paper SPI（ED2208）
#define PIN_EPD_SCK   7
#define PIN_EPD_MOSI  9
#define PIN_EPD_CS    10
#define PIN_EPD_DC    11
#define PIN_EPD_RST   12
#define PIN_EPD_BUSY  13   // アクティブ Low

// ---- 6色パレット（ED2208 ネイティブニブル値） ------------------------------
// shared/layout_contract.md と一致。Orange(0x4) は6色運用で未使用。
#define COLOR_BLACK   0x0
#define COLOR_WHITE   0x1
#define COLOR_YELLOW  0x2
#define COLOR_RED     0x3
#define COLOR_BLUE    0x5
#define COLOR_GREEN   0x6

// ---- バッテリ予約矩形（フォールバック。実行時は manifest 優先） -------------
#define BATTERY_RECT_X  660
#define BATTERY_RECT_Y  448
#define BATTERY_RECT_W  128
#define BATTERY_RECT_H  26

// ---- バッテリ ADC ----------------------------------------------------------
#define PIN_BATT_ADC     1     // GPIO1 (ADC)
#define PIN_BATT_EN      21    // GPIO21 = H で測定有効
#define BATT_DIVIDER     2.0f  // 分圧係数 ×2.0
#define BATT_V_MIN       3.30f // LiPo 曲線
#define BATT_V_MAX       4.20f
#define BATT_LEVELS      5     // 5段階アイコン

// ---- ボタン（アクティブ Low, PULLUP） --------------------------------------
#define PIN_BTN1  3   // 緑
#define PIN_BTN2  4   // 右（EXT1 ウェイク = 手動即時更新）
#define PIN_BTN3  5   // 左

// ---- その他ペリフェラル ----------------------------------------------------
#define PIN_LED    6
#define PIN_BUZZER 45
#define PIN_SD_EN  16
#define PIN_I2C_SCL 20
#define PIN_I2C_SDA 19

// ---- 配信 -----------------------------------------------------------------
// TODO(M5): 実際の GitHub Pages URL に置換
#define DASHBOARD_BASE_URL "https://lab-ish.github.io/reTerminal-weather-dashboard"
#define URL_MANIFEST  DASHBOARD_BASE_URL "/manifest.json"
#define URL_BIN       DASHBOARD_BASE_URL "/dashboard.bin"

// ---- 更新スケジュール（JST 時刻。毎起床で再同期し直近未来へ deep sleep） ----
#define UPDATE_HOURS_JST { 12, 15, 18, 21, 0, 3, 6, 9 }
