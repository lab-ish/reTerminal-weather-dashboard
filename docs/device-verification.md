# 実機動作確認チェックリスト

reTerminal E1002 実機で、表示・ネットワーク・バッテリ・電源管理の各機能を確認・微調整する
ための項目。

**現在の状況**: 実機での目視・機能確認は完了している。表示（6色・レイアウト・フルリフレッシュ）、
ネットワーク（Wi-Fi・HTTPS 取得・展開）、電源管理（SNTP・deep sleep・手動更新）は確認済み。
**未実施はバッテリ ADC（電圧測定）の実測補正のみ**。以下 `[x]` が確認済み、`[ ]` が未実施。

## 事前準備
- [x] `idf.py menuconfig` →「reTerminal Weather」で Wi-Fi SSID/パスワードを設定
- [x] GitHub Pages 有効化済み・`firmware/components/board_cfg/include/app_config.h` の
      `DASHBOARD_BASE_URL` が実 URL と一致
- [x] `idf.py -p <PORT> flash monitor` で書き込み・ログ確認

## 1. e-paper ドライバ
- [x] `fb_test_pattern`（6色縦帯）を表示し、**ニブル値→実際の色**の対応を確認
      （黒0/白1/黄2/赤3/青5/緑6）。ズレたら `cloud/render/palette.py`・
      `firmware/components/board_cfg/include/app_config.h`・`epd_ed2208` を突き合わせ
- [x] 初期化シーケンス（`init_registers`）で正常に立ち上がるか。ゴースト/残像の有無
- [x] フルリフレッシュ実測時間（想定 25〜30s）。`EPD_FULL_REFRESH_TIME_MS` を調整
- [x] SPI クロック `EPD_SPI_HZ`（10MHz）を上げられるか（転送短縮＝省電力）
- 不具合時の保険: arduino-esp32 as component で Seeed_GxEPD2 暫定利用へ切替（二段構え）

## 2. ネットワーク＆展開
- [x] Wi-Fi 接続・**高速再接続**（2回目以降の起床で BSSID 固定が効くか）
- [x] HTTPS で manifest.json / dashboard.bin を取得（`esp_crt_bundle` で github.io 検証）
- [x] `dashboard.bin` が 192000B 取得でき、PSRAM フレームバッファへ展開されるか

## 3. バッテリ＆オーバレイ
- [ ] **（未実施）** GPIO21=H → GPIO1 ADC 実測。`BATT_DIVIDER`（×2.0）・
      LiPo 曲線（`battery_calc.c`）を実測補正
- [x] 予約矩形に「%＋5段階アイコン」が正しい位置・色で重なることを目視確認
      （manifest の battery_rect）。ただし表示される **% 値の正確さは上記 ADC 実測補正待ち**

## 4. 電源管理
- [x] SNTP 同期（JST）と、8時刻の直近未来への deep sleep タイマ設定
- [x] GPIO4（右ボタン）EXT1 ウェイクで手動即時更新できるか
- [x] deep sleep 電流・1サイクル消費・バッテリ持ちの実測

## 5. 消費電力・仕上げ
- [x] 起床予算（目標 <30s。Wi-Fi 2-3s＋DL 1-2s＋リフレッシュ 15-25s＝パネル律速）
- [x] sleep 中に batt EN(GPIO21)/SD(GPIO16)/LED/ブザー OFF、パネル hibernate を確認
- [x] 長時間バッテリ持ちの実測 → 更新頻度の妥当性判断

## 残タスク

- バッテリ ADC（GPIO1）の実測と、`BATT_DIVIDER`・LiPo 電圧曲線（`battery_calc.c`）の
  実測補正。これによりバッテリ % 表示の正確さが確定する。
