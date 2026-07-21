"""2エンドポイントの生 JSON を日付キーでマージし WeatherData を組み立てる。"""

from __future__ import annotations

from datetime import datetime
from typing import Any
from zoneinfo import ZoneInfo

from .. import config
from .models import CurrentWeather, DailyForecast, WeatherData

_JST = ZoneInfo(config.TIMEZONE)


def _now_jst_iso() -> str:
    return datetime.now(_JST).replace(microsecond=0).isoformat()


def merge(jma: dict[str, Any], precip: dict[str, Any]) -> WeatherData:
    """/v1/jma と /v1/forecast をマージ。

    daily.time の並びが両者で一致する前提（同一 latitude/longitude/timezone/
    forecast_days）だが、安全のため precip 側は日付→確率の辞書で突き合わせる。
    """
    cur = jma["current"]
    current = CurrentWeather(
        time_iso=cur["time"],
        temperature=float(cur["temperature_2m"]),
        apparent_temperature=float(cur["apparent_temperature"]),
        humidity=int(round(float(cur["relative_humidity_2m"]))),
        weather_code=int(cur["weather_code"]),
        wind_speed=float(cur["wind_speed_10m"]),
        precipitation=float(cur["precipitation"]),
    )

    jd = jma["daily"]
    dates: list[str] = jd["time"]
    codes = jd["weather_code"]
    tmax = jd["temperature_2m_max"]
    tmin = jd["temperature_2m_min"]

    # 降水確率を日付キーで引けるようにする
    pd = precip.get("daily", {})
    pop_by_date: dict[str, Any] = dict(
        zip(pd.get("time", []), pd.get("precipitation_probability_max", []))
    )

    daily: list[DailyForecast] = []
    for i, date in enumerate(dates):
        pop = pop_by_date.get(date)
        daily.append(
            DailyForecast(
                date=date,
                weather_code=int(codes[i]),
                temp_max=float(tmax[i]),
                temp_min=float(tmin[i]),
                precip_prob=None if pop is None else int(round(float(pop))),
            )
        )

    return WeatherData(
        generated_at=_now_jst_iso(),
        latitude=config.LATITUDE,
        longitude=config.LONGITUDE,
        current=current,
        daily=daily,
    )
