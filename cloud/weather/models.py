"""天気データのドメインモデル（dataclass）。"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class CurrentWeather:
    """現在の天気（Open-Meteo /v1/jma の current ブロック）。"""

    time_iso: str            # 観測/予報時刻（ISO8601, JST）
    temperature: float       # 気温 [°C]
    apparent_temperature: float  # 体感温度 [°C]
    humidity: int            # 相対湿度 [%]
    weather_code: int        # WMO weather code
    wind_speed: float        # 風速 [m/s]
    precipitation: float     # 降水量 [mm]


@dataclass(frozen=True)
class DailyForecast:
    """1日ぶんの週間予報（2エンドポイントをマージ済み）。"""

    date: str                # "YYYY-MM-DD"
    weather_code: int        # WMO weather code
    temp_max: float          # 最高気温 [°C]
    temp_min: float          # 最低気温 [°C]
    wind_speed_max: float    # 最大風速 [m/s]
    precip_prob: int | None  # 降水確率 [%]（/v1/forecast 由来, 欠測時 None）


@dataclass(frozen=True)
class WeatherData:
    """画像生成に必要な全データ。"""

    generated_at: str        # 生成時刻（ISO8601, JST）
    latitude: float
    longitude: float
    current: CurrentWeather
    daily: list[DailyForecast]   # 7日分
