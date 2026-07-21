# セットアップガイド

自分の環境で本ダッシュボードを動かすための通し手順。**地点の変更 → クラウドで画像生成 →
GitHub Pages 公開 → ファーム設定 → ビルド → デバイスへの書き込み → 動作確認**の順で進める。

> 本プロジェクトは処理を2層に分けている。天気データの取得・画像生成は **GitHub Actions（クラウド／Python）**
> が担い、reTerminal 実機（ESP-IDF ファーム）は「完成画像を HTTPS で取得して表示するだけ」。
> したがって **地点変更はクラウド側**、**ビルド・書き込みはファーム側**と担当が分かれる。

## 0. 前提ツール

| ツール | 用途 | 備考 |
|--|--|--|
| Git | リポジトリの取得 | `git clone` |
| Docker | ファームのビルド／書き込み | イメージ `espressif/idf:release-v6.1` を使用 |
| Python 3.12 ＋ pip | クラウド画像生成をローカル確認する場合 | 通常は GitHub Actions が自動実行するので任意 |
| USB シリアル接続 | デバイスへの書き込み | reTerminal E1002 を USB 接続 |
| GitHub アカウント | GitHub Pages 公開 | Actions が生成画像を配信 |

**天気 API キーは不要**。データ源の Open-Meteo は無料・無認証で利用できる。背景画像
（`https://picsum.photos`）も認証不要。

## 1. 表示地点の変更

表示地点は**クラウド側の1ファイルだけ**で決まる。`cloud/config.py` を編集する。

```python
# cloud/config.py（18–20行目）
LATITUDE = 41.8418        # ← 緯度に変更
LONGITUDE = 140.7669      # ← 経度に変更
TIMEZONE = "Asia/Tokyo"   # ← 海外地点なら該当タイムゾーンに変更
```

- 緯度・経度を書き換えるだけでよい。`JMA_PARAMS` / `FORECAST_PARAMS`（30行目以降）は
  これらの定数を参照しているため追加編集は不要。
- 地点は緯度経度ベース（Open-Meteo `/v1/jma`・`/v1/forecast`）。都市名や地域コードは使わない。
- **地点情報はファームに一切持たせていない**（デバイスは画像を受け取るだけ）。
  地点を変えてもファームの焼き直しは不要。
- 注意: 更新スケジュール（`DEVICE_UPDATE_HOURS_JST`）とファーム側の時刻運用は **JST 前提**で
  設計されている。海外地点では表示更新時刻の扱いに注意する。

## 2. クラウド側で画像を生成（ローカル確認・任意）

通常は GitHub Actions が cron で自動生成するため、この手順は動作確認用。

```bash
cd cloud && pip install -r requirements.txt && cd ..
python -m cloud.generate --out ./out
# → out/dashboard.bin (192000 B), out/manifest.json, out/preview.png（目視用）
```

- 依存は `pillow>=10.0` と `requests>=2.31`（`cloud/requirements.txt`）。
- `preview.png` で見た目を確認できる（配信対象外）。`dashboard.bin` が実機へ配る 4bpp 画像。
- データ出典クレジット「Data: Open-Meteo (JMA / multi-model)」（CC BY 4.0）が画像に描画される。

## 3. GitHub Pages で公開

デバイスは GitHub Pages 上の画像を HTTPS で取得する。自分のアカウントで公開する。

1. リポジトリを自分の GitHub アカウントへ置く（fork または push）。
2. **Settings → Pages → Source: GitHub Actions** を選択して有効化。
3. 以後、cron（1日8回）で自動生成・配信される。手動実行は Actions の
   `workflow_dispatch`（`build-dashboard.yml`）から行える。

公開 URL は次の形になる（`<owner>` / `<repo>` は自分の値）。

```
https://<owner>.github.io/<repo>/manifest.json
https://<owner>.github.io/<repo>/dashboard.bin
```

## 4. ファーム設定

### (a) 配信 URL

`firmware/components/board_cfg/include/app_config.h` の `DASHBOARD_BASE_URL`（57行目）を、
手順3で決まった自分の Pages URL に書き換える。

```c
// firmware/components/board_cfg/include/app_config.h（57行目）
#define DASHBOARD_BASE_URL "https://<owner>.github.io/<repo>"
```

### (b) Wi-Fi 認証情報

Wi-Fi の SSID／パスワードは ESP-IDF の menuconfig で入力する（`secrets.h` や `.env` は無い）。

```bash
docker run --rm -v "$PWD/firmware":/project -w /project \
  espressif/idf:release-v6.1 idf.py menuconfig
# メニュー「reTerminal Weather」→ Wi-Fi SSID / Wi-Fi password を入力
```

- 入力値はローカルの `firmware/sdkconfig` に保存される。このファイルは `.gitignore` 済みで
  リポジトリにはコミットされない。**認証情報を平文でコミットしないこと**。

## 5. ビルド

Docker で ESP-IDF を使うのが標準手順。

```bash
docker run --rm -v "$PWD/firmware":/project -w /project \
  espressif/idf:release-v6.1 idf.py build
# → firmware/build/reterminal_weather.bin, firmware/build/merged.bin
```

- ターゲットは `esp32s3`（Flash 32MB / PSRAM 8MB Octal）。
- ローカルに ESP-IDF v6.1 を導入済みなら、`cd firmware && idf.py build` 単体でも可。

## 6. デバイスへの書き込み

reTerminal E1002 を USB で接続し、シリアルポートを指定して書き込む。

```bash
# ローカルに ESP-IDF がある場合（推奨）
cd firmware
idf.py -p <PORT> flash monitor
```

- `<PORT>` はシリアルポート。macOS は `/dev/cu.usbserial-*`、Linux は `/dev/ttyACM0` /
  `/dev/ttyUSB0` 等。
- **Docker でビルドした場合**、フラッシュにはシリアルデバイスをコンテナへ渡す必要がある
  （`--device` オプション）。ホストに ESP-IDF もしくは `esptool` を入れて書き込む方が確実。
- 代替: `esptool` で `firmware/build/merged.bin` を直接書き込む方法もある（同梱の
  `.venv-esptool/` に `esptool` 一式を用意している）。

## 7. 動作確認

- `idf.py monitor`（またはシリアルモニタ）で起動ログを確認する。
- 確認項目: 6色の表示、Wi-Fi 接続、manifest / dashboard.bin の HTTPS 取得、
  フルリフレッシュ（実測 25〜30 秒）。
- **GPIO4（右ボタン）**の EXT1 ウェイクで手動即時更新ができる。
- より詳細な実機チェック項目は [device-verification.md](device-verification.md) を参照。

> 実機での表示・ネットワーク・電源管理は目視・機能確認済み。未実施はバッテリ ADC（電圧測定）の
> 実測補正のみで、バッテリ % 表示の正確さはこの補正で確定する。詳細は
> [device-verification.md](device-verification.md) を参照。

## 関連ドキュメント

- [firmware-porting-notes.md](firmware-porting-notes.md) — e-paper ドライバ（ED2208）の移植メモ
- [device-verification.md](device-verification.md) — 実機動作確認チェックリスト
- `../shared/layout_contract.md` — クラウド ↔ デバイスのレイアウト契約
