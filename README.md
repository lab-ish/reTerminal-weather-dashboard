# reTerminal E1002 天気ダッシュボード

Seeed reTerminal E1002（ESP32-S3, 6色 E Ink Spectra 6, 800×480, 2000mAh）に、
**公立はこだて未来大学**地点の「現在の天気 ＋ 週間予報」を **1日8回**自動更新表示する
低消費電力デバイス。

重い処理（データ取得・レイアウト描画・6色ディザ・4bppパック）はすべて **GitHub Actions**
に寄せ、デバイスは短時間起床で「完成画像の取得と表示」だけを行う。

```
[GitHub Actions cron 1日8回]
  Open-Meteo /v1/jma + /v1/forecast → マージ
  → picsum背景を6色Floyd–Steinbergディザ
  → 不透明ボックス合成 + UIをパレット厳密色で上書き
  → ED2208ネイティブ4bppへパック(dashboard.bin ≈187.5KB) + manifest.json
  → GitHub Pages 公開
            ↓ HTTPS GET
[reTerminal E1002 (ESP-IDF)]
  RTC起床 → バッテリ測定 → Wi-Fi → manifest+bin取得
  → PSRAMフレームバッファ展開 → 予約矩形にバッテリ%重ね書き
  → SPI全画面フルリフレッシュ → deep sleep
```

## リポジトリ構成

| パス | 役割 |
|--|--|
| `cloud/` | 画像生成（Python）。Actions から実行 |
| `cloud/config.py` | 緯度経度・URL・レイアウト座標・出力名の**単一定義源** |
| `cloud/render/palette.py` | 6色パレット / ED2208ニブル値の**単一定義源** |
| `firmware/` | ESP-IDF ファーム（docker `espressif/idf:release-v6.1`） |
| `firmware/main/app_config.h` | ピン・パレット・battery_rect のコンパイル時フォールバック |
| `shared/layout_contract.md` | クラウド↔デバイスの契約（battery_rect・パレット・manifest） |
| `.github/workflows/build-dashboard.yml` | cron・生成・Pages配信 |

## クラウド側（画像生成）

```bash
cd cloud && pip install -r requirements.txt && cd ..
python -m cloud.generate --out ./out
# → out/dashboard.bin (192000 B), out/manifest.json, out/preview.png(目視用)
```

- データ: Open-Meteo `/v1/jma`（気温・天気・現在値）＋ `/v1/forecast`（降水確率）を
  日付キーでマージ。クレジット「Data: Open-Meteo (JMA / multi-model)」（CC BY 4.0）。
- 背景: `https://picsum.photos/800/480` を実行毎にランダム取得し6色ディザ。取得失敗時は無地。
- 表示項目（設計§8準拠・項目を絞る）: 現在=気温(大)/天気アイコン/体感・湿度/日時、
  週間7日=曜日日付/アイコン/最高(赤)最低(青)/降水確率。**週間の風速・現在の降水量mmは非表示**。
- バッテリ欄は無地白の**予約矩形**として出力し、デバイスがローカルで重ね書きする。

## デバイス側（ファーム）

```bash
docker run --rm -v "$PWD/firmware":/project -w /project \
  espressif/idf:release-v6.1 idf.py build
```

コンポーネント: `wifi_conn` `time_sync` `battery` `http_fetch` `epd_ed2208`
`framebuffer` `battery_overlay` `power_mgr`（M0 はスタブ、M4〜M7 で本実装）。

## 更新スケジュール

元データ:30更新 → +15分で Actions（cron遅延を見て安全側 :40）→ +15分でデバイス取得。

- デバイス表示更新（JST）: 12:00, 15:00, 18:00, 21:00, 00:00, 03:00, 06:00, 09:00
- Actions cron（UTC）: `40 2,5,8,11,14,17,20,23 * * *`（＋ `workflow_dispatch` 手動）

## マイルストーン進捗

| | 内容 | 状態 |
|--|--|--|
| M0 | スキャフォールド（構成・config・契約・空ビルド） | ✅ |
| M1 | データ層（fetch + merge） | ✅ |
| M2 | レイアウト＆ディザ（palette/background/layout/icons） | ✅ |
| M3 | パック＆配信（encode + manifest + workflow/cron） | ✅ |
| M4 | e-paperドライバ（Seeed_GxEPD2 → C移植） | ⬜ |
| M5 | ネットワーク＆展開（wifi/http/framebuffer） | ⬜ |
| M6 | バッテリ＆オーバレイ | ⬜ |
| M7 | 電源管理（time_sync/power_mgr/8時刻起床） | ⬜ |
| M8 | 実機検証（オーナーと共同） | ⬜ |
| M9 | 仕上げ | ⬜ |

## 実機セットアップ（M5 以降）

1. GitHub Pages を有効化（Settings → Pages → Source: GitHub Actions）。
2. `firmware/main/app_config.h` の `DASHBOARD_BASE_URL` を実際の Pages URL に合わせる。
3. Wi-Fi 認証情報の投入方法は M5 で決定（NVS / menuconfig など）。
