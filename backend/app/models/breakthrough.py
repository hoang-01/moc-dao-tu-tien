"""Model BreakthroughEvent — Sự kiện đột phá Cảnh Giới."""

import uuid
from datetime import datetime
from typing import TYPE_CHECKING

from sqlalchemy import DateTime, Float, ForeignKey, String, func
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.database import Base

if TYPE_CHECKING:
    from app.models.plant import Plant


class BreakthroughEvent(Base):
    """Bảng breakthrough_events: ghi lại mỗi lần cây đột phá Cảnh Giới.

    - from_rank / to_rank: Tên cảnh giới trước và sau đột phá.
    - exp_at_breakthrough: Tổng Tu Vi tại thời điểm đột phá.
    """

    __tablename__ = "breakthrough_events"

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    plant_id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), ForeignKey("plants.id"), nullable=False, index=True
    )
    from_rank: Mapped[str] = mapped_column(String(50), nullable=False)
    to_rank: Mapped[str] = mapped_column(String(50), nullable=False)
    exp_at_breakthrough: Mapped[float] = mapped_column(Float, nullable=False)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now()
    )

    # Relationships
    plant: Mapped["Plant"] = relationship(  # noqa: F821
        "Plant", back_populates="breakthroughs"
    )
