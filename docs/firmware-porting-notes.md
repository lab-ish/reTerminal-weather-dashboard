# ファーム移植メモ（M4 e-paperドライバ用）

`epd_ed2208` を Arduino ライブラリ **Seeed_GxEPD2** の
`GxEPD2_730c_GDEP073E01`（reTerminal E1002 / パネル GDEP073E01 / コントローラ ED2208）
から C へ移植するための調査結果。ソース実確認済み。

## ソース

| 種別 | URL |
|--|--|
| リポジトリ | https://github.com/Seeed-Projects/Seeed_GxEPD2 （ZinggJM/GxEPD2 のフォーク） |
| ドライバ .h | `src/epd7c/GxEPD2_730c_GDEP073E01.h` |
| ドライバ .cpp | `src/epd7c/GxEPD2_730c_GDEP073E01.cpp` |
| 色定義 | `src/GxEPD2.h`（`GxEPD_*` は RGB565） |
| E1002 例 | `examples/GxEPD2_reTerminal_E1002/GxEPD2_reTerminal_E1002.ino` |

クラス: `GxEPD2_730c_GDEP073E01`（WIDTH=800, HEIGHT=480, hasColor=true, BUSY=アクティブLow）。
このドライバは Spectra 6（6色）を **7色 ACeP テンプレート**で実装しており、コード上は
orange を含む7ニブルを持つ。実運用は6色（orange 0x4 未使用）。

## 色 → 4bit ニブル値（確定）

`_convert_to_native()` と `_colorOfDemoBitmap(mode=0)` の2独立経路が一致（→確定）。

| 色 | パネルニブル | RGB |
|--|--|--|
| Black  | `0x0` | `#000000` |
| White  | `0x1` | `#FFFFFF` |
| Yellow | `0x2` | `#FFFF00` |
| Red    | `0x3` | `#FF0000` |
| (Orange 未使用) | `0x4` | `#FF8000` |
| Blue   | `0x5` | `#0000FF` |
| Green  | `0x6` | `#00FF00` |

パック: 1バイト=2ピクセル（上位ニブル=左, 下位=右）。転送は `writeCommand(0x10)` にニブル列。
→ `shared/layout_contract.md` / `cloud/render/palette.py` / `firmware/main/app_config.h` と一致。

## 初期化・転送・リフレッシュ（すべて .cpp 内）

| 項目 | 関数 | 要点 |
|--|--|--|
| リセット | `_InitDisplay()` 冒頭 | RST: HIGH 50ms → LOW 20ms → HIGH 10ms |
| 初期化 | `_InitDisplay()` | 0xAA(CMDH: 49 55 20 08 09 18), PWRR(0x01)=3F, PSR(0x00)=5F 69, POFS(0x03), BTST1/2/3(0x05/06/08), PLL(0x30)=08, CDI(0x50)=3F, TCON(0x60), TRES(0x61)=0320x01E0, T_VDCS(0x84), PWS(0xE3)=2F → `_PowerOn()` |
| 電源ON | `_PowerOn()` | cmd 0x04 → busy待ち |
| 電源OFF | `_PowerOff()` | cmd 0x02, data 0x00 |
| 画像転送(4bpp) | `writeImage()` / `writeNative()` | cmd 0x10 + `_convert_to_native` |
| バッファ全白 | `writeScreenBuffer()` | cmd 0x10, 0x11 で全面フィル |
| フルリフレッシュ | `refresh(bool)` | 0x50=0x3F(border) → 0x12(DRF) data 0x00 → busy待ち |
| ディープスリープ | `hibernate()` | cmd 0x07, data 0xA5 |

- フルリフレッシュ実測 25–30s（`busy_timeout` は 45s に設定）。**部分更新は不可**（全画面のみ）。
- busy はアクティブ Low。

## 移植方針

1. `_convert_to_native` のスイッチ表を定数テーブル化（GxEPD内部index→パネルニブル）。
   ただしクラウドが既に ED2208 ネイティブ4bppを配信するため、デバイス側は
   **`.bin` をそのまま cmd 0x10 へ流すだけ**でよい（再変換不要）。
2. Arduino 依存（SPI/GPIO）を ESP-IDF の `spi_master` / `gpio` に置換。
3. 保険: ドライバ不具合時は arduino-esp32 as component で Seeed_GxEPD2 を暫定利用（二段構え）。

## 実機で最初に確認すること（M8）

- 6色ベタ表示でニブル→色の対応を確定（クラウドのパレット定義と突き合わせ）。
- ピン（SCK7/MOSI9/CS10/DC11/RST12/BUSY13）・分圧×2.0・LiPo電圧曲線を実測補正。
