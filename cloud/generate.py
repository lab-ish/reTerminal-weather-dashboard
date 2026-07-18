"""ダッシュボード画像生成のオーケストレーション。

fetch → merge → background(picsum+FSディザ) → compose(§8レイアウト) →
encode(ED2208 4bpp) → manifest を実行し、--out ディレクトリへ
dashboard.bin / manifest.json / preview.png を出力する。

実行:
  python -m cloud.generate --out ./out
  python cloud/generate.py --out ./out   # どちらでも可
"""

from __future__ import annotations

# スクリプト直接実行（python cloud/generate.py）でも相対 import を有効にする
if __package__ in (None, ""):
    import os as _os
    import sys as _sys

    _sys.path.insert(0, _os.path.dirname(_os.path.dirname(_os.path.abspath(__file__))))
    __package__ = "cloud"

import argparse
import json
import os

from . import config
from .pack import encode, manifest
from .render import background, dither, layout
from .weather import fetch, merge


def main() -> int:
    ap = argparse.ArgumentParser(description="reTerminal 天気ダッシュボード画像生成")
    ap.add_argument("--out", default="./out", help="出力ディレクトリ")
    ap.add_argument("--no-preview", action="store_true",
                    help="preview.png を出力しない")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)

    print("[1/6] fetch: Open-Meteo /v1/jma + /v1/forecast")
    jma = fetch.fetch_jma()
    precip = fetch.fetch_precip()

    print("[2/6] merge")
    wd = merge.merge(jma, precip)
    print(f"      現在 {wd.current.temperature:.1f}°C / code={wd.current.weather_code} "
          f"/ 湿度{wd.current.humidity}% / 週間{len(wd.daily)}日")

    print("[3/6] background: picsum + Floyd-Steinberg 6色ディザ")
    bg = background.make_background()

    print("[4/6] compose: §8 レイアウト → 6色スナップ")
    img = dither.snap_to_palette(layout.compose(bg, wd))

    print("[5/6] encode: ED2208 4bpp")
    bin_data = encode.encode(img)

    print("[6/6] manifest")
    mf = manifest.build(bin_data, wd)

    bin_path = os.path.join(args.out, config.OUT_BIN)
    mf_path = os.path.join(args.out, config.OUT_MANIFEST)
    with open(bin_path, "wb") as f:
        f.write(bin_data)
    with open(mf_path, "w", encoding="utf-8") as f:
        json.dump(mf, f, ensure_ascii=False, indent=2)

    if not args.no_preview:
        img.save(os.path.join(args.out, config.OUT_PREVIEW))

    print(f"完了: {bin_path} ({len(bin_data)} B), {mf_path}")
    print(f"      content_hash={mf['content_hash'][:16]}...")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
