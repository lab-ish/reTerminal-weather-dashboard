"""背景 + WeatherData から 800x480 の完成 RGB 画像を合成する。

方針: 写真背景のみディザ済み → 不透明ボックスを重ね → UI（文字・アイコン・記号）は
パレット厳密色で上書き（滲み防止）。バッテリ欄は無地白の予約矩形として塗るだけ。
"""

from __future__ import annotations

from datetime import datetime
from functools import lru_cache

from PIL import Image, ImageDraw, ImageFont

from .. import config
from ..weather.models import WeatherData
from . import icons, palette

# アイコン種別 -> 現在天気キャプション（DejaVu = Latin のため英語表記）
_CAPTION = {
    icons.CLEAR: "Clear",
    icons.PARTLY: "Partly Cloudy",
    icons.CLOUDY: "Overcast",
    icons.FOG: "Fog",
    icons.DRIZZLE: "Drizzle",
    icons.RAIN: "Rain",
    icons.SNOW: "Snow",
    icons.SHOWER: "Showers",
    icons.SNOW_SHOWER: "Snow Showers",
    icons.THUNDER: "Thunder",
}


@lru_cache(maxsize=None)
def _font(path: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(path, size)


def _reg(size: int) -> ImageFont.FreeTypeFont:
    return _font(config.FONT_REGULAR, size)


def _bold(size: int) -> ImageFont.FreeTypeFont:
    return _font(config.FONT_BOLD, size)


def _text_size(d: ImageDraw.ImageDraw, text: str, font) -> tuple[int, int]:
    l, t, r, b = d.textbbox((0, 0), text, font=font)
    return r - l, b - t


def _text(d, xy, text, font, fill, anchor="la") -> None:
    d.text(xy, text, font=font, fill=fill, anchor=anchor)


def _draw_current(d: ImageDraw.ImageDraw, wd: WeatherData) -> None:
    x, y, w, h = config.CURRENT_BOX
    # 不透明白ボックス
    d.rounded_rectangle((x, y, x + w, y + h), radius=10, fill=palette.C_WHITE)

    cur = wd.current
    gen = datetime.fromisoformat(wd.generated_at)

    # 日付・時刻（左上）
    _text(d, (x + 22, y + 16), gen.strftime("%Y.%m.%d"),
          _reg(config.FS_DATE), palette.C_BLACK)
    _text(d, (x + 220, y + 16), gen.strftime("%H:%M"),
          _reg(config.FS_TIME), palette.C_BLACK)

    # 天気アイコン（左）＋キャプション（白ボックス内に収める）
    icon_s = 76
    ix, iy = x + 24, y + 50
    icons.draw_icon(d, cur.weather_code, ix, iy, icon_s)
    kind = icons.weather_kind(cur.weather_code)
    _text(d, (ix + icon_s / 2, iy + icon_s + 2), _CAPTION.get(kind, ""),
          _reg(config.FS_SMALL), palette.C_BLACK, anchor="ma")

    # 現在気温（特大）＋単位
    temp_txt = f"{cur.temperature:.1f}"
    tf = _bold(config.FS_TEMP_BIG)
    _text(d, (x + 150, y + 52), temp_txt, tf, palette.C_BLACK)
    tw, _ = _text_size(d, temp_txt, tf)
    _text(d, (x + 150 + tw + 12, y + 112), "(°C)",
          _reg(config.FS_LABEL), palette.C_BLACK)

    # 右: 黒パネル（湿度 + 体感温度、風速、降水量mm）
    px, py, pw, ph = config.CURRENT_RIGHT_PANEL
    d.rounded_rectangle((px, py, px + pw, py + ph), radius=0, fill=palette.C_BLACK)
    # 湿度
    _text(d, (px + pw / 6, py + 8), f"{cur.humidity:.0f}",
          _bold(config.FS_PANEL_BIG), palette.C_WHITE, anchor="ma")
    _text(d, (px + pw / 6, py + 50), "Humidity",
          _reg(config.FS_SMALL), palette.C_WHITE, anchor="ma")
    _text(d, (px + pw / 6, py + 71), "(%)",
          _reg(config.FS_SMALL), palette.C_WHITE, anchor="ma")
    # 風速
    _text(d, (px + pw / 2, py + 8), f"{cur.wind_speed:.1f}",
          _bold(config.FS_PANEL_BIG), palette.C_WHITE, anchor="ma")
    _text(d, (px + pw / 2, py + 50), "Wind Speed",
          _reg(config.FS_SMALL), palette.C_WHITE, anchor="ma")
    _text(d, (px + pw / 2, py + 71), "(m/s)",
          _reg(config.FS_SMALL), palette.C_WHITE, anchor="ma")
    # 降水量
    _text(d, (px + pw * 5 / 6, py + 8), f"{cur.precipitation:.1f}",
          _bold(config.FS_PANEL_BIG), palette.C_WHITE, anchor="ma")
    _text(d, (px + pw * 5 / 6, py + 50), "Rain",
          _reg(config.FS_SMALL), palette.C_WHITE, anchor="ma")
    _text(d, (px + pw * 5 / 6, py + 71), "(mm)",
          _reg(config.FS_SMALL), palette.C_WHITE, anchor="ma")
    # 体感温度
    _text(d, (px + pw / 3 - 3, py + 110), "Feels like (°C)",
          _reg(config.FS_SMALL), palette.C_BLACK, anchor="ma")
    _text(d, (px + pw * 2 / 3 + 3, py + 100), f"{cur.apparent_temperature:.1f}",
          _bold(config.FS_PANEL_BIG), palette.C_BLACK, anchor="ma")


def _draw_weekly(d: ImageDraw.ImageDraw, wd: WeatherData) -> None:
    top = config.WEEKLY_TOP
    hh = config.WEEKLY_HEADER_H
    cw, ch = config.CARD_W, config.CARD_H
    for i, day in enumerate(wd.daily[:7]):
        cx = config.card_x(i)
        # カード白ボディ
        d.rounded_rectangle((cx, top, cx + cw, top + ch), radius=8,
                            fill=palette.C_WHITE)
        # 曜日の黒ヘッダ
        d.rounded_rectangle((cx, top, cx + cw, top + hh), radius=8,
                            fill=palette.C_BLACK)
        d.rectangle((cx, top + hh - 10, cx + cw, top + hh), fill=palette.C_BLACK)
        dt = datetime.strptime(day.date, "%Y-%m-%d")
        _text(d, (cx + cw / 2, top + hh / 2), dt.strftime("%a"),
              _bold(config.FS_WEEKDAY), palette.C_WHITE, anchor="mm")
        # 日付
        _text(d, (cx + cw / 2, top + hh + 6), dt.strftime("%m/%d"),
              _reg(config.FS_CARD_DATE), palette.C_BLACK, anchor="ma")
        # アイコン
        icon_s = 58
        icons.draw_icon(d, day.weather_code, cx + (cw - icon_s) / 2,
                        top + hh + 28, icon_s)
        # 最高/最低気温（赤/青）
        ty = top + hh + 100
        _text(d, (cx + cw * 0.30, ty), f"{day.temp_max:.0f}",
              _bold(config.FS_CARD_TEMP), palette.C_RED, anchor="ma")
        _text(d, (cx + cw * 0.70, ty), f"{day.temp_min:.0f}",
              _bold(config.FS_CARD_TEMP), palette.C_BLUE, anchor="ma")
        # 降水確率（青い水滴 + %）
        py = ty + 22
        pop = "--" if day.precip_prob is None else f"{day.precip_prob}"
        d.chord((cx + 14, py + 12, cx + 34, py + 32), start=180, end=0, outline=palette.C_BLACK)
        d.line((cx + 24, py + 22, cx + 24, py + 32), fill=palette.C_BLACK)
        d.arc((cx + 18, py + 29, cx + 24, py + 35), start=0, end=180, fill=palette.C_BLACK)
        _text(d, (cx + cw * 0.95, py + 22), f"{pop} %",
              _reg(config.FS_CARD_POP), palette.C_BLACK, anchor="rm")
        # 最大風速
        wy = py + 40
        d.line((cx + 6, wy + 12, cx + 26, wy + 12), fill="black", width=1)
        d.arc((cx + 22, wy + 4, cx + 30, wy + 12), start=180, end=90, fill="black")
        d.line((cx + 9, wy + 15, cx + 29, wy + 15), fill="black", width=1)
        d.arc((cx + 25, wy + 15, cx + 33, wy + 23), start=270, end=180, fill="black")
        _text(d, (cx + cw * 0.95, wy + 12), f"{day.wind_speed_max:.0f} m/s",
              _reg(config.FS_CARD_POP), palette.C_BLACK, anchor="rm")

def _draw_footer(d: ImageDraw.ImageDraw) -> None:
    # クレジット（左）: 背景の明暗に依らず読めるよう黒縁取り
    d.text(config.CREDIT_POS, config.CREDIT_TEXT, font=_reg(config.FS_CREDIT),
           fill=palette.C_WHITE, anchor="la",
           stroke_width=2, stroke_fill=palette.C_BLACK)
    # バッテリ予約矩形（無地白）: デバイスが % + 5段階アイコンを重ね書きする
    bx, by, bw, bh = config.BATTERY_RECT
    d.rectangle((bx, by, bx + bw, by + bh), fill=palette.C_WHITE)


def compose(background: Image.Image, wd: WeatherData) -> Image.Image:
    """完成 RGB 画像を返す（パレット厳密色のみ）。"""
    img = background.convert("RGB").copy()
    d = ImageDraw.Draw(img)
    _draw_current(d, wd)
    _draw_weekly(d, wd)
    _draw_footer(d)
    return img
