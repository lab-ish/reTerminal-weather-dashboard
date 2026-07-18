"""Open-Meteo の2エンドポイントを取得する（リトライ + キャッシュフォールバック）。

失敗時は直近の良好レスポンス（cache/ 配下の JSON）を利用する。両方失敗した場合は
例外を送出し、呼び出し側（generate.py）が Pages 上書きを中止する。
"""

from __future__ import annotations

import json
import os
import time
from typing import Any

import requests

from .. import config

_CACHE_DIR = os.path.join(os.path.dirname(__file__), "..", "cache")


def _cache_path(name: str) -> str:
    return os.path.join(_CACHE_DIR, f"{name}.json")


def _save_cache(name: str, data: dict[str, Any]) -> None:
    os.makedirs(_CACHE_DIR, exist_ok=True)
    with open(_cache_path(name), "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False)


def _load_cache(name: str) -> dict[str, Any] | None:
    path = _cache_path(name)
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def _get_json(url: str, params: dict[str, Any], cache_name: str) -> dict[str, Any]:
    """リトライ付き GET。失敗時はキャッシュへフォールバック。"""
    last_err: Exception | None = None
    for attempt in range(1, config.HTTP_RETRIES + 1):
        try:
            resp = requests.get(url, params=params, timeout=config.HTTP_TIMEOUT)
            resp.raise_for_status()
            data = resp.json()
            _save_cache(cache_name, data)
            return data
        except Exception as e:  # noqa: BLE001 - ネットワーク/JSON 全般をリトライ対象にする
            last_err = e
            if attempt < config.HTTP_RETRIES:
                time.sleep(config.HTTP_BACKOFF * attempt)

    cached = _load_cache(cache_name)
    if cached is not None:
        print(f"[fetch] {cache_name}: 取得失敗のためキャッシュを使用 ({last_err})")
        return cached
    raise RuntimeError(f"{cache_name} の取得に失敗し、キャッシュも無い: {last_err}")


def fetch_jma() -> dict[str, Any]:
    """気温・天気・現在値（/v1/jma）。"""
    return _get_json(config.JMA_URL, config.JMA_PARAMS, "jma")


def fetch_precip() -> dict[str, Any]:
    """降水確率（/v1/forecast, best_match）。"""
    return _get_json(config.FORECAST_URL, config.FORECAST_PARAMS, "forecast")
