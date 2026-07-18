"""天気アイコンを Pillow でパレット厳密色（黒/白/黄/赤/青/緑）で描画する。

外部SVG素材に依存せず、フラットな図形をパレット色で直接描く（ディザリング不要）。
WMO weather code をグルーピングしてアイコン種別へ対応させる。

各 draw_* は (x, y) 左上・一辺 s の正方領域に描画する。天気アイコンは白ボックス/
白カード上に配置される前提で、雲は白塗り+黒輪郭とする。
"""

from __future__ import annotations

from PIL import ImageDraw

from . import palette

# WMO code -> アイコン種別
CLEAR = "clear"
PARTLY = "partly"
CLOUDY = "cloudy"
FOG = "fog"
DRIZZLE = "drizzle"
RAIN = "rain"
SNOW = "snow"
SHOWER = "shower"
SNOW_SHOWER = "snow_shower"
THUNDER = "thunder"


def weather_kind(code: int) -> str:
    if code == 0:
        return CLEAR
    if code in (1, 2):
        return PARTLY
    if code == 3:
        return CLOUDY
    if code in (45, 48):
        return FOG
    if 51 <= code <= 57:
        return DRIZZLE
    if 61 <= code <= 67:
        return RAIN
    if 71 <= code <= 77:
        return SNOW
    if 80 <= code <= 82:
        return SHOWER
    if code in (85, 86):
        return SNOW_SHOWER
    if 95 <= code <= 99:
        return THUNDER
    return CLOUDY


def _sun(d: ImageDraw.ImageDraw, x: float, y: float, r: float, rays: bool = True) -> None:
    cx, cy = x, y
    if rays:
        import math

        for i in range(8):
            a = math.pi / 4 * i
            x1 = cx + math.cos(a) * (r + r * 0.35)
            y1 = cy + math.sin(a) * (r + r * 0.35)
            x2 = cx + math.cos(a) * (r + r * 0.95)
            y2 = cy + math.sin(a) * (r + r * 0.95)
            d.line((x1, y1, x2, y2), fill=palette.C_YELLOW, width=max(2, int(r * 0.18)))
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=palette.C_YELLOW,
              outline=palette.C_YELLOW)


def _cloud(d: ImageDraw.ImageDraw, x: float, y: float, s: float,
           fill=palette.C_WHITE, outline=palette.C_BLACK) -> tuple[float, float]:
    """雲を描き、雲底の (left_x, bottom_y) を返す（降水記号の起点用）。"""
    w = s
    h = s * 0.62
    ox = x
    oy = y + s * 0.12
    lw = max(2, int(s * 0.05))
    # 3つの丸 + 底の角丸長方形
    d.ellipse((ox + w * 0.02, oy + h * 0.30, ox + w * 0.46, oy + h * 0.98),
              fill=fill, outline=outline, width=lw)
    d.ellipse((ox + w * 0.22, oy + h * 0.02, ox + w * 0.72, oy + h * 0.72),
              fill=fill, outline=outline, width=lw)
    d.ellipse((ox + w * 0.52, oy + h * 0.28, ox + w * 0.98, oy + h * 0.98),
              fill=fill, outline=outline, width=lw)
    d.rounded_rectangle((ox + w * 0.06, oy + h * 0.60, ox + w * 0.94, oy + h * 0.98),
                        radius=int(h * 0.18), fill=fill)
    # 内側の輪郭線を消す（底面の水平線を白で上書き）
    d.line((ox + w * 0.12, oy + h * 0.96, ox + w * 0.88, oy + h * 0.96),
           fill=fill, width=lw)
    return ox + w * 0.12, oy + h * 0.98


def _rain_streaks(d: ImageDraw.ImageDraw, left: float, top: float, s: float,
                  n: int = 3, color=palette.C_BLUE) -> None:
    lw = max(2, int(s * 0.05))
    for i in range(n):
        sx = left + s * (0.18 + i * 0.28)
        d.line((sx, top, sx - s * 0.08, top + s * 0.22), fill=color, width=lw)


def _snow_marks(d: ImageDraw.ImageDraw, left: float, top: float, s: float,
                n: int = 3, color=palette.C_BLUE) -> None:
    r = max(2, int(s * 0.045))
    for i in range(n):
        sx = left + s * (0.18 + i * 0.28)
        sy = top + s * 0.10
        d.ellipse((sx - r, sy - r, sx + r, sy + r), fill=color)


def _bolt(d: ImageDraw.ImageDraw, cx: float, top: float, s: float) -> None:
    d.polygon(
        [
            (cx + s * 0.02, top),
            (cx - s * 0.12, top + s * 0.20),
            (cx + s * 0.00, top + s * 0.20),
            (cx - s * 0.10, top + s * 0.40),
            (cx + s * 0.14, top + s * 0.14),
            (cx + s * 0.02, top + s * 0.14),
        ],
        fill=palette.C_YELLOW,
        outline=palette.C_RED,
    )


def draw_icon(d: ImageDraw.ImageDraw, code: int, x: float, y: float, s: float) -> None:
    """WMO code に応じたアイコンを (x,y) 左上・一辺 s で描画する。"""
    kind = weather_kind(code)

    if kind == CLEAR:
        _sun(d, x + s * 0.5, y + s * 0.5, s * 0.30)
        return

    if kind == PARTLY:
        _sun(d, x + s * 0.68, y + s * 0.34, s * 0.20)
        _cloud(d, x + s * 0.04, y + s * 0.22, s * 0.80)
        return

    if kind == CLOUDY or kind == FOG:
        _cloud(d, x + s * 0.06, y + s * 0.12, s * 0.86)
        if kind == FOG:
            lw = max(2, int(s * 0.05))
            for i in range(3):
                ly = y + s * 0.72 + i * s * 0.10
                d.line((x + s * 0.14, ly, x + s * 0.86, ly),
                       fill=palette.C_BLACK, width=lw)
        return

    # 降水系: 雲 + 記号
    left, bottom = _cloud(d, x + s * 0.06, y + s * 0.02, s * 0.86)
    top = bottom + s * 0.02
    width = s * 0.74
    if kind in (DRIZZLE, RAIN, SHOWER):
        n = 2 if kind == DRIZZLE else 3
        _rain_streaks(d, left, top, width, n=n)
    elif kind in (SNOW, SNOW_SHOWER):
        _snow_marks(d, left, top, width, n=3)
    elif kind == THUNDER:
        _bolt(d, x + s * 0.5, top, s * 0.5)
        _rain_streaks(d, left, top, width, n=2)
