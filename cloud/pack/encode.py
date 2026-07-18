"""完成 RGB 画像を ED2208 ネイティブ 4bpp（2px/byte）へパックする。

パック仕様（palette.py と一致）:
  - 1バイト = 2ピクセル
  - 上位ニブル = 左（偶数 x）ピクセル / 下位ニブル = 右（奇数 x）ピクセル
  - ニブル値は palette.NIBBLE_OF_RGB（黒0x0/白0x1/黄0x2/赤0x3/青0x5/緑0x6）
出力サイズ = WIDTH * HEIGHT / 2（800x480 → 192000 B ≈ 187.5KB）。
"""

from __future__ import annotations

from PIL import Image

from .. import config
from ..render import palette


def encode(img: Image.Image) -> bytes:
    if img.size != (config.WIDTH, config.HEIGHT):
        raise ValueError(f"想定外の画像サイズ: {img.size}")
    if config.WIDTH % 2 != 0:
        raise ValueError("WIDTH は偶数である必要がある")

    pixels = list(img.convert("RGB").getdata())
    nib_of = palette.NIBBLE_OF_RGB
    w, h = config.WIDTH, config.HEIGHT

    out = bytearray(w * h // 2)
    oi = 0
    for row in range(h):
        base = row * w
        for col in range(0, w, 2):
            lp = pixels[base + col]
            rp = pixels[base + col + 1]
            ln = nib_of.get(lp)
            if ln is None:
                ln = palette.nearest_nibble(lp)
            rn = nib_of.get(rp)
            if rn is None:
                rn = palette.nearest_nibble(rp)
            out[oi] = (ln << 4) | rn
            oi += 1
    return bytes(out)
