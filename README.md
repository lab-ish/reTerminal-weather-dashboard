# reTerminal E1002 天気ダッシュボード

Seeed reTerminal E1002（ESP32-S3, 6色 E Ink Spectra 6, 800×480, 2000mAh）に、任意地点の
「現在の天気 ＋ 週間予報」を **1日8回**自動更新表示する低消費電力デバイス。

重い処理（データ取得・レイアウト描画・6色ディザ・4bppパック）はすべて **GitHub Actions**
に寄せ、デバイスは短時間起床で「完成画像の取得と表示」だけを行う。

```
[GitHub Actions cron 1日8回]
  Open-Meteo /v1/jma + /v1/forecast → マージ
  → picsum背景を6色Floyd–Steinbergディザ
  → 不透明ボックス合成 + UIをパレット厳密色で上書き
  → ED2208ネイティブ4bppへパック(dashboard.bin = 192000 B) + manifest.json
  → GitHub Pages 公開
            ↓ HTTPS GET
[reTerminal E1002 (ESP-IDF)]
  RTC起床 → バッテリ測定 → Wi-Fi → manifest+bin取得
  → PSRAMフレームバッファ展開 → 予約矩形にバッテリ%重ね書き
  → SPI全画面フルリフレッシュ → deep sleep
```

## セットアップ

自分の環境で動かす通し手順（**地点変更・ビルド・デバイスへの書き込み**）は
**[docs/setup-guide.md](docs/setup-guide.md)** にまとめている。要点は次のとおり。

1. **表示地点の変更** — `cloud/config.py` の `LATITUDE` / `LONGITUDE`（18–19行目）を編集。
   地点情報はファームに持たせないため、地点変更でファームの焼き直しは不要。
2. **GitHub Pages 公開** — リポジトリを自分のアカウントへ置き、Settings → Pages →
   Source: GitHub Actions を有効化。cron で画像が自動生成・配信される。
3. **ファーム設定** — `firmware/components/board_cfg/include/app_config.h` の
   `DASHBOARD_BASE_URL`（57行目）を自分の Pages URL に。Wi-Fi は `idf.py menuconfig` で入力。
4. **ビルド & 書き込み** — Docker ＋ ESP-IDF でビルドし、USB 接続の実機へフラッシュ。

> 天気 API キーは不要（Open-Meteo は無料・無認証）。

## リポジトリ構成

| パス | 役割 |
|--|--|
| `cloud/` | 画像生成（Python）。Actions から実行 |
| `cloud/config.py` | 緯度経度・URL・レイアウト座標・出力名の**単一定義源** |
| `cloud/render/palette.py` | 6色パレット / ED2208ニブル値の**単一定義源** |
| `firmware/` | ESP-IDF ファーム（docker `espressif/idf:release-v6.1`） |
| `firmware/components/board_cfg/include/app_config.h` | ピン・パレット・battery_rect のコンパイル時フォールバック |
| `shared/layout_contract.md` | クラウド↔デバイスの契約（battery_rect・パレット・manifest） |
| `.github/workflows/build-dashboard.yml` | cron・生成・Pages配信 |
| `docs/` | セットアップ・移植メモ・実機動作確認 |

## クイックスタート

### クラウド側（画像生成・ローカル確認）

```bash
cd cloud && pip install -r requirements.txt && cd ..
python -m cloud.generate --out ./out
# → out/dashboard.bin (192000 B), out/manifest.json, out/preview.png（目視用）
```

- データ: Open-Meteo `/v1/jma`（気温・天気・現在値）＋ `/v1/forecast`（降水確率）を
  日付キーでマージ。
- 背景: `https://picsum.photos/800/480` を実行毎にランダム取得し6色ディザ。取得失敗時は無地。
- 表示項目: 現在=気温(大)/天気アイコン/体感・湿度/日時、週間7日=曜日日付/アイコン/
  最高(赤)最低(青)/降水確率。
- バッテリ欄は無地白の**予約矩形**として出力し、デバイスがローカルで重ね書きする。

### デバイス側（ファーム）

```bash
# Wi-Fi 認証情報を設定（初回のみ）
docker run --rm -v "$PWD/firmware":/project -w /project \
  espressif/idf:release-v6.1 idf.py menuconfig   # reTerminal Weather → SSID/Password
# ビルド
docker run --rm -v "$PWD/firmware":/project -w /project \
  espressif/idf:release-v6.1 idf.py build
# 書き込み（ホストに ESP-IDF がある場合。<PORT> はシリアルポート）
cd firmware && idf.py -p <PORT> flash monitor
```

コンポーネント: `board_cfg`(ピン/パレット/URL 定数) `wifi_conn`(STA+RTC高速再接続)
`time_sync`(SNTP+JST) `battery`(ADC+×2.0+LiPo曲線) `http_fetch`(HTTPS+crt_bundle,
manifest軽量パース) `epd_ed2208`(SPIドライバ) `framebuffer`(PSRAM 4bpp) `battery_overlay`
(予約矩形へ%+5段階アイコン直書き) `power_mgr`(8時刻起床+deep sleep)。起床フローは
`main/main.c`。

詳細な手順とトラブルシュートは [docs/setup-guide.md](docs/setup-guide.md)、実機での確認項目は
[docs/device-verification.md](docs/device-verification.md) を参照。

## 更新スケジュール

元データ:30更新 → +10分で Actionsを実行すると良い（デフォルトでは無効化されている）。
デバイスでは:00に取得。

Github Actionsをcron実行すると実行時刻が大きくバラつくため、 [cron-job.org](https://cron-job.org/) などの利用を推奨する。
設定方法は [docs/cron-job-guide.md](docs/cron-job-guide.md) を参照。

> スケジュールと時刻表示は JST 前提で設計されている。海外地点で使う場合は時刻運用に注意。

## データ出典・ライセンス

- 天気データ: [Open-Meteo](https://open-meteo.com/)（JMA / multi-model）。**CC BY 4.0**。
  生成画像にクレジット「Data: Open-Meteo (JMA / multi-model)」を描画している。
- 背景画像: [Lorem Picsum](https://picsum.photos/)（実行毎にランダム取得）。
- フォント: DejaVu Sans（再現性のためリポジトリに同梱）。

## License

This software is released under the BSD 3-clause license. See `LICENSE.txt`.

* Copyright (c) 2026, Shigemi ISHIDA

## Third-Party Fonts

This software bundles the DejaVu Sans font (https://dejavu-fonts.github.io/).
DejaVu Sans is released under its own license, which includes portions copyrighted by Bitstream, Inc. (Bitstream Vera Fonts) and Tavmjong Bah (Arev Fonts glyphs), with DejaVu's own modifications dedicated to the public domain.
See `DejaVu-LICENSE.txt` for the full license text.
