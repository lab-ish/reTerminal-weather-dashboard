"""背景画像（picsum）を取得し、800x480・6色ディザ済み RGB を返す。

取得失敗時は無地フォールバック（白）。天気ゾーンは後段で不透明ボックスに完全被覆
されるため、背景の可読性リスクはボックス外の端のみ。
"""

from __future__ import annotations

import io

import requests
from PIL import Image

from .. import config
from . import dither, palette


def _fetch_picsum() -> Image.Image | None:
    try:
        resp = requests.get(config.PICSUM_URL, timeout=config.HTTP_TIMEOUT)
        resp.raise_for_status()
        return Image.open(io.BytesIO(resp.content)).convert("RGB")
    except Exception as e:  # noqa: BLE001
        print(f"[background] picsum 取得失敗、無地フォールバック: {e}")
        return None


def _normalize(img: Image.Image) -> Image.Image:
    """アスペクト比を保って 800x480 にセンタークロップ。"""
    tw, th = config.WIDTH, config.HEIGHT
    iw, ih = img.size
    scale = max(tw / iw, th / ih)
    nw, nh = int(round(iw * scale)), int(round(ih * scale))
    img = img.resize((nw, nh), Image.LANCZOS)
    left = (nw - tw) // 2
    top = (nh - th) // 2
    return img.crop((left, top, left + tw, top + th))


def make_background() -> Image.Image:
    """6色ディザ済みの背景 RGB 画像を返す。"""
    src = _fetch_picsum()
    if src is None:
        return Image.new("RGB", (config.WIDTH, config.HEIGHT), palette.C_WHITE)
    return dither.dither_to_palette(_normalize(src))
