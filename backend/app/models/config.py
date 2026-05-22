"""Config Models — PlantType (loại cây), RankConfig (cảnh giới), ExpConfig (hệ số Tu Vi)."""

import uuid
from datetime import datetime

from sqlalchemy import DateTime, Float, Integer, String, Text, func
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import Mapped, mapped_column

from app.database import Base


class PlantType(Base):
    """Bảng plant_types: định nghĩa các loại cây và ngưỡng lý tưởng.

    Admin có thể CRUD và chỉnh ngưỡng lý tưởng qua giao diện.
    Mỗi metric có min/max xác định khoảng lý tưởng.
    """

    __tablename__ = "plant_types"

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    name: Mapped[str] = mapped_column(String(100), unique=True, nullable=False)
    description: Mapped[str | None] = mapped_column(Text, nullable=True)

    # Ngưỡng lý tưởng — Độ ẩm đất (%)
    soil_moisture_min: Mapped[float] = mapped_column(Float, default=40.0)
    soil_moisture_max: Mapped[float] = mapped_column(Float, default=70.0)

    # Ngưỡng lý tưởng — Ánh sáng (lux)
    light_min: Mapped[float] = mapped_column(Float, default=1000.0)
    light_max: Mapped[float] = mapped_column(Float, default=10000.0)

    # Ngưỡng lý tưởng — Nhiệt độ (°C)
    temperature_min: Mapped[float] = mapped_column(Float, default=20.0)
    temperature_max: Mapped[float] = mapped_column(Float, default=30.0)

    # Ngưỡng lý tưởng — Độ ẩm không khí (%)
    humidity_min: Mapped[float] = mapped_column(Float, default=50.0)
    humidity_max: Mapped[float] = mapped_column(Float, default=80.0)

    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now()
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), onupdate=func.now()
    )


class RankConfig(Base):
    """Bảng rank_configs: định nghĩa các Cảnh Giới và mốc Tu Vi.

    Admin có thể điều chỉnh mốc Tu Vi mà không cần deploy lại.
    """

    __tablename__ = "rank_configs"

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    order: Mapped[int] = mapped_column(Integer, unique=True, nullable=False)
    name: Mapped[str] = mapped_column(String(50), unique=True, nullable=False)
    min_exp: Mapped[float] = mapped_column(Float, nullable=False, default=0.0)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now()
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), onupdate=func.now()
    )


class ExpConfig(Base):
    """Bảng exp_configs: cấu hình hệ số cộng/trừ Tu Vi theo mức môi trường.

    - quality_level: EXCELLENT, GOOD, FAIR, POOR, DANGER
    - exp_delta: Số điểm cộng (dương) hoặc trừ (âm) mỗi chu kỳ.
    """

    __tablename__ = "exp_configs"

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    quality_level: Mapped[str] = mapped_column(String(20), unique=True, nullable=False)
    exp_delta: Mapped[float] = mapped_column(Float, nullable=False)
    description: Mapped[str | None] = mapped_column(Text, nullable=True)
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), onupdate=func.now()
    )
