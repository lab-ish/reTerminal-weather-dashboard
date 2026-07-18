"""manifest.json を生成する。

デバイスへ配布する唯一の source of truth: battery_rect（予約矩形座標）と palette
（nibble→RGB）を含める。content_hash は将来のリフレッシュ省略用に含めるが初版未使用。
"""

from __future__ import annotations

import hashlib

from .. import config
from ..render import palette
from ..weather.models import WeatherData


def build(bin_data: bytes, wd: WeatherData) -> dict:
    bx, by, bw, bh = config.BATTERY_RECT
    return {
        "version": config.MANIFEST_VERSION,
        "generated_at": wd.generated_at,               # ISO8601 (JST)
        "width": config.WIDTH,
        "height": config.HEIGHT,
        "format": config.IMAGE_FORMAT,                  # "ed2208_4bpp"
        "bytes": len(bin_data),
        "palette": [
            {"nibble": c.nibble, "rgb": list(c.rgb), "name": c.name}
            for c in palette.COLORS
        ],
        "battery_rect": {"x": bx, "y": by, "w": bw, "h": bh},
        "content_hash": hashlib.sha256(bin_data).hexdigest(),
    }
