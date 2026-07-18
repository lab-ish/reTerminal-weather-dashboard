"""単一定義源（single source of truth）。

緯度経度・API URL・更新スケジュール・レイアウト座標・出力ファイル名など、
クラウド側画像生成の全定数をここに集約する。バッテリ予約矩形(battery_rect)は
manifest.json 経由でデバイスへ配布され、ファーム側 app_config.h のフォールバック値と
一致させる（レイアウト変更でファーム焼き直しを不要にするため）。

色（6色パレット / ニブルインデックス）は render/palette.py を唯一の定義源とする。
"""

from __future__ import annotations

import os

# ---------------------------------------------------------------------------
# 予測地点（公立はこだて未来大学）
# ---------------------------------------------------------------------------
LATITUDE = 41.8418
LONGITUDE = 140.7669
TIMEZONE = "Asia/Tokyo"
FORECAST_DAYS = 7

# ---------------------------------------------------------------------------
# Open-Meteo API（無料・無認証）
#   ① 気温・天気・現在値 → /v1/jma
#   ② 降水確率           → /v1/forecast（best_match アンサンブル）
# daily.time（日付配列）をキーに突き合わせマージする。
# ---------------------------------------------------------------------------
JMA_URL = "https://api.open-meteo.com/v1/jma"
JMA_PARAMS = {
    "latitude": LATITUDE,
    "longitude": LONGITUDE,
    "current": "temperature_2m,apparent_temperature,relative_humidity_2m,"
    "weather_code,wind_speed_10m",
    "daily": "weather_code,temperature_2m_max,temperature_2m_min",
    "timezone": TIMEZONE,
    "forecast_days": FORECAST_DAYS,
}

FORECAST_URL = "https://api.open-meteo.com/v1/forecast"
FORECAST_PARAMS = {
    "latitude": LATITUDE,
    "longitude": LONGITUDE,
    "daily": "precipitation_probability_max",
    "timezone": TIMEZONE,
    "forecast_days": FORECAST_DAYS,
}

# データ出典クレジット（CC BY 4.0 対応）
CREDIT_TEXT = "Data: Open-Meteo (JMA / multi-model)"

# 背景画像（Actions 実行毎にランダム取得）
PICSUM_URL = "https://picsum.photos/800/480"

# HTTP リトライ
HTTP_TIMEOUT = 15          # 秒
HTTP_RETRIES = 3
HTTP_BACKOFF = 2.0         # 秒（指数バックオフの基数）

# ---------------------------------------------------------------------------
# デバイス表示更新スケジュール（JST）。GitHub Actions cron は各時刻の 20 分前
# （UTC 40 分）に発火する（元データ:30更新 → +15 分で Actions を安全側:40 に前倒し）。
#   cron: "40 2,5,8,11,14,17,20,23 * * *"  (UTC) = JST 11:40,14:40,...,08:40
# ---------------------------------------------------------------------------
DEVICE_UPDATE_HOURS_JST = [12, 15, 18, 21, 0, 3, 6, 9]

# ---------------------------------------------------------------------------
# キャンバス（E Ink Spectra 6, 800x480）
# ---------------------------------------------------------------------------
WIDTH = 800
HEIGHT = 480
MARGIN = 12

# --- 現在の天気ゾーン（上部・不透明白ボックス） -----------------------------
# (x, y, w, h)
CURRENT_BOX = (MARGIN, 44, WIDTH - 2 * MARGIN, 156)   # 12,44,776,156 → y:44..200
# 左サブゾーン: 日付・時刻(上) / 大きな現在気温 + 天気アイコン
# 右サブゾーン: 黒パネル（湿度 + 体感温度）。参考PNGの風速/降水量mmは非表示。
CURRENT_RIGHT_PANEL = (500, 56, 276, 132)             # 黒パネル 500,56 .. 776,188

# --- 週間予報ゾーン（7カード） --------------------------------------------
WEEKLY_TOP = 212
CARD_W = 104
CARD_GAP = 8
CARD_H = 226                                           # 212..438
WEEKLY_HEADER_H = 28                                   # 曜日の黒ヘッダ高さ


def card_x(i: int) -> int:
    """i 番目（0-6）の週間カード左端 x 座標。"""
    return MARGIN + i * (CARD_W + CARD_GAP)


# --- フッター（クレジット + バッテリ予約矩形） -----------------------------
FOOTER_Y = 446
CREDIT_POS = (MARGIN, 454)

# バッテリ予約矩形（デバイスが % + 5段階アイコンをローカル重ね書きする領域）。
# クラウドはここを無地（白）で塗るだけ。manifest.json に埋めてデバイスへ配布する。
# (x, y, w, h)
BATTERY_RECT = (WIDTH - MARGIN - 128, 448, 128, 26)    # 660,448,128,26 → 660..788

# ---------------------------------------------------------------------------
# フォント（再現性のため DejaVu をリポジトリに同梱）
# ---------------------------------------------------------------------------
_ASSETS = os.path.join(os.path.dirname(__file__), "assets")
FONT_REGULAR = os.path.join(_ASSETS, "fonts", "DejaVuSans.ttf")
FONT_BOLD = os.path.join(_ASSETS, "fonts", "DejaVuSans-Bold.ttf")
ICONS_DIR = os.path.join(_ASSETS, "icons")

# 主要フォントサイズ
FS_TEMP_BIG = 92        # 現在気温（特大）
FS_DATE = 30            # 日付
FS_TIME = 30            # 時刻
FS_LABEL = 20           # 汎用ラベル
FS_SMALL = 16           # 補助
FS_PANEL_BIG = 34       # 黒パネルの数値（湿度・体感）
FS_WEEKDAY = 20         # 曜日ヘッダ
FS_CARD_DATE = 15       # カード内日付
FS_CARD_TEMP = 20       # カード内 最高/最低気温
FS_CARD_POP = 16        # カード内 降水確率
FS_CREDIT = 13          # フッタークレジット

# ---------------------------------------------------------------------------
# 出力
# ---------------------------------------------------------------------------
MANIFEST_VERSION = 1
OUT_BIN = "dashboard.bin"
OUT_MANIFEST = "manifest.json"
OUT_PREVIEW = "preview.png"       # 目視検証用（配信対象外）
IMAGE_FORMAT = "ed2208_4bpp"      # manifest.format
