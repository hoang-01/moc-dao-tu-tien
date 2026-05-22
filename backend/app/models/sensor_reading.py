"""Model SensorReading — Dữ liệu cảm biến (time-series)."""

import uuid
from datetime import datetime
from typing import TYPE_CHECKING

from sqlalchemy import DateTime, Float, ForeignKey, Index, String, func
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.database import Base

if TYPE_CHECKING:
    from app.models.plant import Plant


class SensorReading(Base):
    """Bảng sensor_readings: lưu lịch sử dữ liệu cảm biến.

    - sensor_key: "soil_moisture" | "light" | "temperature" | "humidity"
    - quality: "EXCELLENT" | "GOOD" | "FAIR" | "POOR" | "DANGER"
    """

    __tablename__ = "sensor_readings"
    __table_args__ = (
        Index("ix_sensor_readings_plant_created", "plant_id", "created_at"),
        Index(
            "ix_sensor_readings_plant_key_created",
            "plant_id",
            "sensor_key",
            "created_at",
        ),
    )

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    plant_id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), ForeignKey("plants.id"), nullable=False
    )
    sensor_key: Mapped[str] = mapped_column(String(50), nullable=False)
    value: Mapped[float] = mapped_column(Float, nullable=False)
    quality: Mapped[str] = mapped_column(String(20), nullable=False)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), index=True
    )

    # Relationships
    plant: Mapped["Plant"] = relationship(  # noqa: F821
        "Plant", back_populates="sensor_readings"
    )
