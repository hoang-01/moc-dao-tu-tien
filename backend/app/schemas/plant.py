"""Schemas Plant — Request/Response cho quản lý cây."""

import uuid
from datetime import datetime

from pydantic import BaseModel


class PlantPairRequest(BaseModel):
    """Body liên kết chậu cây: Plant Code + Verify Code."""

    plant_code: str
    verify_code: str
    name: str
    plant_type_id: uuid.UUID


class PlantUpdateRequest(BaseModel):
    """Body cập nhật thông tin cây."""

    name: str | None = None
    plant_type_id: uuid.UUID | None = None


class PlantTypeInfo(BaseModel):
    """Thông tin loại cây (dùng trong response)."""

    id: uuid.UUID
    name: str

    model_config = {"from_attributes": True}


class RankInfo(BaseModel):
    """Thông tin Cảnh Giới (dùng trong response)."""

    id: uuid.UUID
    order: int
    name: str
    min_exp: float

    model_config = {"from_attributes": True}


class SensorStatus(BaseModel):
    """Trạng thái 1 cảm biến trên Dashboard."""

    sensor_key: str
    value: float
    quality: str
    updated_at: datetime | None


class DashboardResponse(BaseModel):
    """Response Dashboard: chỉ số, Tu Vi, Cảnh Giới, tiến trình."""

    plant_id: uuid.UUID
    plant_name: str
    plant_type: PlantTypeInfo

    # Tu Vi & Cảnh Giới
    total_exp: float
    current_rank: RankInfo
    next_rank: RankInfo | None
    exp_to_next_rank: float | None

    # Chỉ số cảm biến hiện tại
    sensors: list[SensorStatus]
    overall_quality: str

    # Trạng thái thiết bị
    device_online: bool
    device_last_seen: datetime | None


class SensorHistoryPoint(BaseModel):
    """Một điểm dữ liệu trong biểu đồ lịch sử."""

    value: float
    quality: str
    created_at: datetime


class PlantHistoryResponse(BaseModel):
    """Response lịch sử cảm biến cho biểu đồ xu hướng."""

    sensor_key: str
    readings: list[SensorHistoryPoint]
    ideal_min: float
    ideal_max: float


class PlantResponse(BaseModel):
    """Response thông tin cây cơ bản."""

    id: uuid.UUID
    name: str
    plant_type: PlantTypeInfo
    total_exp: float
    current_rank: RankInfo
    created_at: datetime

    model_config = {"from_attributes": True}
