// ホスト単体テスト（ESP-IDF非依存の純ロジック）。
//   cc -I ... test_host.c fb_pixels.c battery_calc.c power_calc.c battery_overlay.c
// バッテリ%/段数・次回起床秒・ニブル操作を assert し、overlay 結果を .bin へ出力する。
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "battery_calc.h"
#include "battery_overlay.h"
#include "fb_pixels.h"
#include "power_calc.h"

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); fails++; } \
                              else { printf("ok  : %s\n", msg); } } while (0)

static void test_battery_calc(void)
{
    CHECK(batt_voltage_to_percent(4.30f) == 100, "batt 4.30V -> 100%");
    CHECK(batt_voltage_to_percent(4.20f) == 100, "batt 4.20V -> 100%");
    CHECK(batt_voltage_to_percent(3.30f) == 0,   "batt 3.30V -> 0%");
    CHECK(batt_voltage_to_percent(3.20f) == 0,   "batt 3.20V -> 0%");
    int p38 = batt_voltage_to_percent(3.80f);
    CHECK(p38 == 50, "batt 3.80V -> 50%");
    int p395 = batt_voltage_to_percent(3.95f);  // 3.90(65)〜4.00(80) の中間 -> ~72.5
    CHECK(p395 >= 70 && p395 <= 75, "batt 3.95V -> ~72%");

    CHECK(batt_percent_to_bucket(0, 5) == 0,   "bucket 0% -> 0");
    CHECK(batt_percent_to_bucket(1, 5) == 1,   "bucket 1% -> 1 (最低1点灯)");
    CHECK(batt_percent_to_bucket(100, 5) == 5, "bucket 100% -> 5");
    CHECK(batt_percent_to_bucket(50, 5) >= 2 && batt_percent_to_bucket(50,5) <= 3,
          "bucket 50% -> 2..3");
    CHECK(batt_percent_to_bucket(80, 5) == 4, "bucket 80% -> 4");
}

static void test_power_calc(void)
{
    int hours[] = UPDATE_HOURS_JST;   // 12,15,18,21,0,3,6,9
    int n = sizeof(hours) / sizeof(hours[0]);

    // JST 2026-07-18 12:00:00 = UTC 03:00:00。次は 15:00 → 3h=10800s
    // UTC epoch for 2026-07-18T03:00:00Z:
    long base = 1784343600L;  // 下で検証
    // 動的計算に頼らず、JST正午ちょうど → 直近未来15:00 まで10800s を確認
    long secs = seconds_to_next_wake(base, hours, n);
    printf("    next wake from base = %ld s\n", secs);
    CHECK(secs == 10800, "JST12:00 -> next 15:00 (10800s)");

    // 1分前（JST 11:59）→ 12:00 まで 60s
    CHECK(seconds_to_next_wake(base - 60, hours, n) == 60, "JST11:59 -> 12:00 (60s)");
    // JST 21:00 ちょうど → 次は 00:00（同時刻は除外, 3h）
    CHECK(seconds_to_next_wake(base + 9*3600, hours, n) == 10800,
          "JST21:00 -> next 00:00 (10800s)");
    // すべて 0<secs<=24h
    for (long t = base; t < base + 86400; t += 777) {
        long s = seconds_to_next_wake(t, hours, n);
        assert(s > 0 && s <= 86400);
    }
    CHECK(1, "next wake は常に 0<s<=24h");
}

static void test_fb_pixels(void)
{
    uint8_t *buf = calloc(EPD_FB_BYTES, 1);
    fb_set_nibble(buf, 0, 0, COLOR_RED);
    fb_set_nibble(buf, 1, 0, COLOR_BLUE);
    CHECK(fb_get_nibble(buf, 0, 0) == COLOR_RED,  "nibble (0,0)=red 往復");
    CHECK(fb_get_nibble(buf, 1, 0) == COLOR_BLUE, "nibble (1,0)=blue 往復");
    // 1バイト目 = 上位=red(3) 下位=blue(5) = 0x35
    CHECK(buf[0] == ((COLOR_RED << 4) | COLOR_BLUE), "pack 上位=左");

    fb_fill(buf, COLOR_WHITE);
    CHECK(fb_get_nibble(buf, 123, 45) == COLOR_WHITE, "fill white");

    fb_test_pattern(buf);
    CHECK(fb_get_nibble(buf, 0, 0) == COLOR_BLACK,    "band0 black");
    CHECK(fb_get_nibble(buf, 799, 0) == COLOR_GREEN,  "band5 green");
    free(buf);
}

static void write_overlay_bin(void)
{
    uint8_t *buf = calloc(EPD_FB_BYTES, 1);
    fb_test_pattern(buf);   // 6色帯（M8色確認）
    // battery_rect に複数サンプルを縦に並べて描画（視認用）
    battery_overlay_draw(buf, BATTERY_RECT_X, BATTERY_RECT_Y,
                         BATTERY_RECT_W, BATTERY_RECT_H, 73, 4, BATT_LEVELS);
    battery_overlay_draw(buf, 20, 20, BATTERY_RECT_W, BATTERY_RECT_H, 100, 5, BATT_LEVELS);
    battery_overlay_draw(buf, 20, 60, BATTERY_RECT_W, BATTERY_RECT_H, 8, 1, BATT_LEVELS);
    battery_overlay_draw(buf, 20, 100, BATTERY_RECT_W, BATTERY_RECT_H, 45, 3, BATT_LEVELS);

    // overlay 後、矩形が白背景+黒描画（帯の上に白で塗り直されている）を確認
    CHECK(fb_get_nibble(buf, BATTERY_RECT_X + 1, BATTERY_RECT_Y + 1) == COLOR_WHITE,
          "overlay 矩形は白で塗り直し");

    FILE *f = fopen("/tmp/overlay_test.bin", "wb");
    fwrite(buf, 1, EPD_FB_BYTES, f);
    fclose(f);
    printf("    wrote /tmp/overlay_test.bin (%d B)\n", EPD_FB_BYTES);
    free(buf);
}

int main(void)
{
    test_battery_calc();
    test_power_calc();
    test_fb_pixels();
    write_overlay_bin();
    printf("\n%s (fails=%d)\n", fails ? "NG" : "ALL PASS", fails);
    return fails ? 1 : 0;
}
