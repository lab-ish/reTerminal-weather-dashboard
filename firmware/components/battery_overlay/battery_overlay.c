// battery_overlay 実装（M6）: 予約矩形に % と 5段階アイコンをニブル直書き。
#include "battery_overlay.h"

#include "app_config.h"
#include "fb_pixels.h"

// 5x7 ビットマップフォント（bit4..bit0 = 左..右）。数字と '%'。
static const uint8_t FONT_DIGIT[10][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
};
static const uint8_t FONT_PCT[7] = {0x18,0x19,0x02,0x04,0x08,0x13,0x03}; // %

// 5x7 グリフを (x,y) に scale 倍で描く。
static void draw_glyph(uint8_t *buf, int x, int y, const uint8_t *g, int scale, uint8_t nib)
{
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (g[row] & (1 << (4 - col))) {
                fb_fill_rect(buf, x + col * scale, y + row * scale, scale, scale, nib);
            }
        }
    }
}

// 文字送り幅（scale倍のグリフ + 1*scale スペース）
static int glyph_advance(int scale) { return (5 + 1) * scale; }

static void draw_number(uint8_t *buf, int x, int y, int value, int scale, uint8_t nib)
{
    char tmp[4];
    int n = 0;
    if (value < 0) value = 0;
    if (value > 999) value = 999;
    // 10進を逆順に
    do { tmp[n++] = (char)('0' + value % 10); value /= 10; } while (value && n < 3);
    for (int i = n - 1; i >= 0; i--) {
        draw_glyph(buf, x, y, FONT_DIGIT[tmp[i] - '0'], scale, nib);
        x += glyph_advance(scale);
    }
}

// バッテリアイコン（外枠 + 端子 + bucket段の塗り）を (x,y,w,h) に描く。
static void draw_battery_icon(uint8_t *buf, int x, int y, int w, int h,
                              int bucket, int levels)
{
    const int nub = 3;
    int bw = w - nub;
    // 外枠（2px 黒）
    fb_fill_rect(buf, x, y, bw, 2, COLOR_BLACK);
    fb_fill_rect(buf, x, y + h - 2, bw, 2, COLOR_BLACK);
    fb_fill_rect(buf, x, y, 2, h, COLOR_BLACK);
    fb_fill_rect(buf, x + bw - 2, y, 2, h, COLOR_BLACK);
    // 端子（右）
    fb_fill_rect(buf, x + bw, y + h / 4, nub, h / 2, COLOR_BLACK);
    // セグメント
    if (levels < 1) levels = 1;
    int inner_x = x + 3;
    int inner_y = y + 3;
    int inner_w = bw - 6;
    int inner_h = h - 6;
    int gap = 1;
    int seg_w = (inner_w - (levels - 1) * gap) / levels;
    for (int i = 0; i < bucket && i < levels; i++) {
        fb_fill_rect(buf, inner_x + i * (seg_w + gap), inner_y, seg_w, inner_h,
                     COLOR_BLACK);
    }
}

void battery_overlay_draw(uint8_t *buf, int rx, int ry, int rw, int rh,
                          int percent, int bucket, int levels)
{
    // 予約矩形を白で塗り直し（クリーンスレート）
    fb_fill_rect(buf, rx, ry, rw, rh, COLOR_WHITE);

    const int scale = 2;                  // 5x7 → 10x14
    const int gh = 7 * scale;             // グリフ高
    int ty = ry + (rh - gh) / 2;
    int tx = rx + 2;

    // "NN%"
    draw_number(buf, tx, ty, percent, scale, COLOR_BLACK);
    int digits = (percent >= 100) ? 3 : (percent >= 10 ? 2 : 1);
    tx += digits * glyph_advance(scale);
    draw_glyph(buf, tx, ty, FONT_PCT, scale, COLOR_BLACK);
    tx += glyph_advance(scale);

    // アイコン（矩形の右側に配置）
    int icon_w = 38;
    int icon_h = rh - 6;
    int icon_x = rx + rw - icon_w - 2;
    int icon_y = ry + 3;
    if (icon_x < tx + 4) icon_x = tx + 4;   // 文字と重ならないよう最低限確保
    draw_battery_icon(buf, icon_x, icon_y, icon_w, icon_h, bucket, levels);
}
