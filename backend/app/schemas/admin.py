"""Schemas Admin — Request/Response cho Admin Dashboard & Configuration."""

import uuid
from datetime import datetime

from pydantic import BaseModel


# --- Admin Dashboard ---
class AdminDashboardResponse(BaseModel):
    """Thống kê tổng quan hệ thống cho Admin."""

    total_users: int
    total_devices: int
    devices_online: int
    devices_offline: int
    total_plants_paired: int
    rank_distribution: list["RankDistribution"]
    new_users_daily: list["DailyStat"]


class RankDistribution(BaseModel):
    """Phân bố Cảnh Giới trong hệ thống."""

    rank_name: str
    count: int


class DailyStat(BaseModel):
    """Thống kê theo ngày."""

    date: str
    count: int


# --- Plant Type Management ---
class PlantTypeCreateRequest(BaseModel):
    """Body tạo loại cây mới."""

    name: str
    description: str | None = None
    soil_moisture_min: float = 40.0
    soil_moisture_max: float = 70.0
    light_min: float = 1000.0
    light_max: float = 10000.0
    temperature_min: float = 20.0
    temperature_max: float = 30.0
    humidity_min: float = 50.0
    humidity_max: float = 80.0


class PlantTypeUpdateRequest(BaseModel):
    """Body cập nhật loại cây."""

    name: str | None = None
    description: str | None = None
    soil_moisture_min: float | None = None
    soil_moisture_max: float | None = None
    light_min: float | None = None
    light_max: float | None = None
    temperature_min: float | None = None
    temperature_max: float | None = None
    humidity_min: float | None = None
    humidity_max: float | None = None


class PlantTypeResponse(BaseModel):
    """Response thông tin loại cây đầy đủ."""

    id: uuid.UUID
    name: str
    description: str | None
    soil_moisture_min: float
    soil_moisture_max: float
    light_min: float
    light_max: float
    temperature_min: float
    temperature_max: float
    humidity_min: float
    humidity_max: float
    created_at: datetime
    updated_at: datetime

    model_config = {"from_attributes": True}


# --- EXP Config ---
class ExpConfigItem(BaseModel):
    """Một dòng cấu hình hệ số Tu Vi."""

    id: uuid.UUID
    quality_level: str
    exp_delta: float
    description: str | None

    model_config = {"from_attributes": True}


class ExpConfigUpdateRequest(BaseModel):
    """Body cập nhật hệ số Tu Vi."""

    configs: list["ExpConfigUpdateItem"]


class ExpConfigUpdateItem(BaseModel):
    """Một dòng cập nhật."""

    quality_level: str
    exp_delta: float
    description: str | None = None


# --- Rank Config ---
class RankConfigItem(BaseModel):
    """Một Cảnh Giới trong danh sách."""

    id: uuid.UUID
    order: int
    name: str
    min_exp: float

    model_config = {"from_attributes": True}


class RankConfigUpdateRequest(BaseModel):
    """Body cập nhật danh sách Cảnh Giới."""

    ranks: list["RankConfigUpdateItem"]


class RankConfigUpdateItem(BaseModel):
    """Một dòng cập nhật Cảnh Giới."""

    order: int
    name: str
    min_exp: float
