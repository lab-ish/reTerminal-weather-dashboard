# レイアウト契約書（クラウド ↔ デバイス）

クラウド（`cloud/`）とファーム（`firmware/`）が共有する **唯一の契約**。値の変更は
`cloud/config.py`・`cloud/render/palette.py` を編集し、本ファイルと `firmware/main/app_config.h`
を同期させること。実行時はクラウドが `manifest.json` に埋めてデバイスへ配布するため、
レイアウト変更でファームを焼き直す必要はない（app_config.h はフォールバック値）。

## 画像フォーマット

| 項目 | 値 |
|--|--|
| 解像度 | 800 × 480 |
| フォーマット | `ed2208_4bpp`（ED2208 ネイティブ 4bpp） |
| パック | 1バイト = 2ピクセル。**上位ニブル = 左（偶数 x）**, 下位ニブル = 右（奇数 x） |
| バイト数 | 800 × 480 ÷ 2 = **192000 B** |
| 転送 | コントローラへ `writeCommand(0x10)` でニブル列を送出 |

## 6色パレット（ニブル値）

Seeed_GxEPD2 `GxEPD2_730c_GDEP073E01`（`_convert_to_native` / `_colorOfDemoBitmap`）から
確定。Orange(0x4) は7色テンプレート由来で **6色運用では未使用**。

| 色 | ニブル | RGB |
|--|--|--|
| Black  | `0x0` | `#000000` |
| White  | `0x1` | `#FFFFFF` |
| Yellow | `0x2` | `#FFFF00` |
| Red    | `0x3` | `#FF0000` |
| (Orange 未使用) | `0x4` | `#FF8000` |
| Blue   | `0x5` | `#0000FF` |
| Green  | `0x6` | `#00FF00` |

## バッテリ予約矩形（battery_rect）

デバイスが「% + 5段階アイコン」をローカル重ね書きする領域。クラウドは無地白で塗るだけ。

| x | y | w | h |
|--|--|--|--|
| 660 | 448 | 128 | 26 |

（`cloud/config.py: BATTERY_RECT` と一致。manifest.json の `battery_rect` が実行時の権威、
app_config.h はフォールバック。）

## manifest.json スキーマ

```json
{
  "version": 1,
  "generated_at": "2026-07-18T10:36:00+09:00",
  "width": 800,
  "height": 480,
  "format": "ed2208_4bpp",
  "bytes": 192000,
  "palette": [{"nibble": 0, "rgb": [0,0,0], "name": "black"}, ...],
  "battery_rect": {"x": 660, "y": 448, "w": 128, "h": 26},
  "content_hash": "<sha256 of dashboard.bin>"
}
```

`content_hash` は将来のリフレッシュ省略（前回と同一ならフルリフレッシュを回避）用。初版未使用。

## 配信 URL（GitHub Pages）

```
https://<owner>.github.io/<repo>/manifest.json
https://<owner>.github.io/<repo>/dashboard.bin
```

デバイスは manifest → bin の順に GET し、常に「Pages 上の最新」を取得する。
