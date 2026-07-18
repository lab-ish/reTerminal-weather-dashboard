#include "fb_pixels.h"

#include "app_config.h"   // EPD_WIDTH / EPD_HEIGHT / COLOR_* （純粋な #define のみ）

static const uint8_t kBandColors[6] = {
    COLOR_BLACK, COLOR_WHITE, COLOR_YELLOW, COLOR_RED, COLOR_BLUE, COLOR_GREEN,
};

void fb_set_nibble(uint8_t *buf, int x, int y, uint8_t nib)
{
    if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) return;
    size_t idx = ((size_t)y * EPD_WIDTH + x) / 2;
    nib &= 0x0F;
    if (x & 1) buf[idx] = (buf[idx] & 0xF0) | nib;
    else       buf[idx] = (buf[idx] & 0x0F) | (nib << 4);
}

uint8_t fb_get_nibble(const uint8_t *buf, int x, int y)
{
    if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) return 0;
    size_t idx = ((size_t)y * EPD_WIDTH + x) / 2;
    return (x & 1) ? (buf[idx] & 0x0F) : (buf[idx] >> 4);
}

void fb_fill(uint8_t *buf, uint8_t nib)
{
    nib &= 0x0F;
    uint8_t b = (nib << 4) | nib;
    for (size_t i = 0; i < (size_t)EPD_FB_BYTES; i++) buf[i] = b;
}

void fb_fill_rect(uint8_t *buf, int x, int y, int w, int h, uint8_t nib)
{
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            fb_set_nibble(buf, xx, yy, nib);
}

void fb_test_pattern(uint8_t *buf)
{
    const int band = EPD_WIDTH / 6;
    for (int y = 0; y < EPD_HEIGHT; y++) {
        for (int x = 0; x < EPD_WIDTH; x++) {
            int b = x / band;
            if (b > 5) b = 5;
            fb_set_nibble(buf, x, y, kBandColors[b]);
        }
    }
}
