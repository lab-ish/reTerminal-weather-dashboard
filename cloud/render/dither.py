"""RGB 画像を6色パレットへ Floyd-Steinberg ディザリングする。

写真背景にのみ適用する。UI要素（アイコン・文字・不透明ボックス）はディザせず
パレット厳密色で上書きするため、この関数は background のみに使う。
"""

from __future__ import annotations

from PIL import Image

from . import palette


def _palette_image() -> Image.Image:
    pal = Image.new("P", (1, 1))
    pal.putpalette(palette.palette_flat())
    return pal


def dither_to_palette(img: Image.Image) -> Image.Image:
    """RGB 画像を6色 Floyd-Steinberg でディザし、RGB（パレット厳密色のみ）で返す。"""
    rgb = img.convert("RGB")
    pal = _palette_image()
    quantized = rgb.quantize(palette=pal, dither=Image.Dither.FLOYDSTEINBERG)
    # convert("RGB") でパレット厳密色に戻す（後段の合成・エンコードで最近傍探索が不要になる）
    return quantized.convert("RGB")


def snap_to_palette(img: Image.Image) -> Image.Image:
    """完成画像を6色へ最近傍スナップ（ディザなし）し RGB で返す。

    UI（文字・アイコン）のアンチエイリアス中間色を厳密パレット色へ丸める。
    実機（6色・AA不可）と完全一致するプレビュー/エンコード入力を得るための最終段。
    """
    rgb = img.convert("RGB")
    pal = _palette_image()
    quantized = rgb.quantize(palette=pal, dither=Image.Dither.NONE)
    return quantized.convert("RGB")
