# M8 実機検証チェックリスト（オーナーと共同）

本Macは実機未接続のため、M4〜M7 は「実装・ビルド・ホスト単体テスト」まで完了。
以下は実機（reTerminal E1002）で確認・微調整する項目。

## 事前準備
- [ ] `idf.py menuconfig` →「reTerminal Weather」で Wi-Fi SSID/パスワードを設定
- [ ] GitHub Pages 有効化済み・`app_config.h` の `DASHBOARD_BASE_URL` が実URLと一致
- [ ] `idf.py -p <PORT> flash monitor` で書き込み・ログ確認

## 1. e-paper ドライバ（M4）
- [ ] `fb_test_pattern`（6色縦帯）を表示し、**ニブル値→実際の色**の対応を確認
      （黒0/白1/黄2/赤3/青5/緑6）。ズレたら `palette.py`・`app_config.h`・`epd_ed2208` を突き合わせ
- [ ] 初期化シーケンス（`init_registers`）で正常に立ち上がるか。ゴースト/残像の有無
- [ ] フルリフレッシュ実測時間（想定25〜30s）。`EPD_FULL_REFRESH_TIME_MS` を調整
- [ ] SPIクロック `EPD_SPI_HZ`(10MHz) を上げられるか（転送短縮＝省電力）
- [ ] 不具合時は arduino-esp32 as component で Seeed_GxEPD2 暫定利用へ切替（二段構え）

## 2. ネットワーク＆展開（M5）
- [ ] Wi-Fi 接続・**高速再接続**（2回目以降の起床で BSSID固定が効くか）
- [ ] HTTPS で manifest.json / dashboard.bin を取得（`esp_crt_bundle` で github.io 検証）
- [ ] `dashboard.bin` が 192000B 取得でき、PSRAM フレームバッファへ展開されるか

## 3. バッテリ＆オーバレイ（M6）
- [ ] GPIO21=H → GPIO1 ADC 実測。`BATT_DIVIDER`(×2.0)・LiPo曲線(`battery_calc.c`)を実測補正
- [ ] 予約矩形に「%＋5段階アイコン」が正しい位置・色で重なるか（manifest の battery_rect）

## 4. 電源管理（M7）
- [ ] SNTP 同期（JST）と、8時刻の直近未来への deep sleep タイマ設定
- [ ] GPIO4（右ボタン）EXT1 ウェイクで手動即時更新できるか
- [ ] deep sleep 電流・1サイクル消費・バッテリ持ちの実測

## 5. 消費電力・仕上げ（M9へ）
- [ ] 起床予算（目標 <30s。Wi-Fi 2-3s＋DL 1-2s＋リフレッシュ15-25s＝パネル律速）
- [ ] sleep 中に batt EN(GPIO21)/SD(GPIO16)/LED/ブザー OFF、パネル hibernate を確認
- [ ] 長時間バッテリ持ちの実測 → 更新頻度の妥当性判断
