"""E Ink Spectra 6（GDEP073E01 / ED2208）の6色パレット。唯一の定義源。

ニブル割り当ては Seeed_GxEPD2 の GxEPD2_730c_GDEP073E01（`_convert_to_native`,
`_colorOfDemoBitmap`）から抽出・確定した値。ファーム側 epd_ed2208 の色定数と
`shared/layout_contract.md` はこの表と一致させること。

パック仕様: 1バイト = 2ピクセル（上位ニブル=左ピクセル, 下位ニブル=右ピクセル）。
コントローラへは writeCommand(0x10) でこのニブル列を転送する。

Orange(0x4) は 7色テンプレート由来でパネル上は6色運用のため使用しない
（ディザ・UI描画の対象パレットから除外する）。
"""

from __future__ import annotations

from typing import NamedTuple

RGB = tuple[int, int, int]


class Color(NamedTuple):
    name: str
    nibble: int
    rgb: RGB


# 6色（運用色）。ニブル値は ED2208 パネルネイティブ値。
BLACK = Color("black", 0x0, (0x00, 0x00, 0x00))
WHITE = Color("white", 0x1, (0xFF, 0xFF, 0xFF))
YELLOW = Color("yellow", 0x2, (0xFF, 0xFF, 0x00))
RED = Color("red", 0x3, (0xFF, 0x00, 0x00))
BLUE = Color("blue", 0x5, (0x00, 0x00, 0xFF))
GREEN = Color("green", 0x6, (0x00, 0xFF, 0x00))

# Orange(0x4) は非運用（参考用に定義のみ保持、パレットには含めない）
ORANGE = Color("orange", 0x4, (0xFF, 0x80, 0x00))

# 運用パレット（ディザ・量子化・エンコードの対象）
COLORS: list[Color] = [BLACK, WHITE, YELLOW, RED, BLUE, GREEN]

# 描画で使う RGB ショートカット
C_BLACK = BLACK.rgb
C_WHITE = WHITE.rgb
C_YELLOW = YELLOW.rgb
C_RED = RED.rgb
C_BLUE = BLUE.rgb
C_GREEN = GREEN.rgb

# rgb -> nibble（完成画像を4bppパックする際の厳密対応）
NIBBLE_OF_RGB: dict[RGB, int] = {c.rgb: c.nibble for c in COLORS}

# nibble -> rgb（manifest 配布用 / デコード検証用）
RGB_OF_NIBBLE: dict[int, RGB] = {c.nibble: c.rgb for c in COLORS}


def palette_flat() -> list[int]:
    """Pillow の P モード用フラットパレット（256色ぶんにパディング）。

    先頭 len(COLORS) エントリが運用6色。残りは先頭色で埋める。
    """
    flat: list[int] = []
    for c in COLORS:
        flat.extend(c.rgb)
    # 256色ぶん (768値) までパディング
    pad = COLORS[0].rgb
    while len(flat) < 256 * 3:
        flat.extend(pad)
    return flat


def nearest_nibble(rgb: RGB) -> int:
    """最近傍の運用色のニブル値を返す（完成画像に想定外色が混じった場合の保険）。"""
    best = COLORS[0]
    best_d = 1 << 30
    r, g, b = rgb
    for c in COLORS:
        cr, cg, cb = c.rgb
        d = (r - cr) ** 2 + (g - cg) ** 2 + (b - cb) ** 2
        if d < best_d:
            best_d = d
            best = c
    return best.nibble
