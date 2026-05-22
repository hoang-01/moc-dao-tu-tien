"""Model ExpLog — Nhật ký thay đổi điểm Tu Vi."""

import uuid
from datetime import datetime
from typing import TYPE_CHECKING

from sqlalchemy import DateTime, Float, ForeignKey, String, func
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.database import Base

if TYPE_CHECKING:
    from app.models.plant import Plant


class ExpLog(Base):
    """Bảng exp_logs: ghi lại mỗi lần cộng/trừ Tu Vi.

    - delta: Số điểm thay đổi (+/-).
    - reason: Mức môi trường gây ra thay đổi.
    - overall_quality: Mức tổng hợp tại thời điểm tính.
    """

    __tablename__ = "exp_logs"

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    plant_id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), ForeignKey("plants.id"), nullable=False, index=True
    )
    delta: Mapped[float] = mapped_column(Float, nullable=False)
    reason: Mapped[str] = mapped_column(String(50), nullable=False)
    overall_quality: Mapped[str] = mapped_column(String(20), nullable=False)
    total_exp_after: Mapped[float] = mapped_column(Float, nullable=False)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now()
    )

    # Relationships
    plant: Mapped["Plant"] = relationship("Plant", back_populates="exp_logs")  # noqa: F821
