"""Model Device — Thiết bị IoT với Plant Code & Verify Code."""

import uuid
from datetime import datetime
from typing import TYPE_CHECKING

from sqlalchemy import Boolean, DateTime, String, func
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.database import Base

if TYPE_CHECKING:
    from app.models.plant import Plant


class Device(Base):
    """Bảng devices: quản lý thiết bị IoT.

    - plant_code: Mã định danh 8 ký tự, in trên thiết bị.
    - verify_hash: bcrypt hash của Verify Code (6 số) dùng để xác thực liên kết.
    """

    __tablename__ = "devices"

    id: Mapped[uuid.UUID] = mapped_column(
        UUID(as_uuid=True), primary_key=True, default=uuid.uuid4
    )
    plant_code: Mapped[str] = mapped_column(
        String(16), unique=True, nullable=False, index=True
    )
    verify_hash: Mapped[str] = mapped_column(String(128), nullable=False)
    is_paired: Mapped[bool] = mapped_column(Boolean, default=False, nullable=False)
    is_active: Mapped[bool] = mapped_column(Boolean, default=True, nullable=False)
    last_seen_at: Mapped[datetime | None] = mapped_column(
        DateTime(timezone=True), nullable=True
    )
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now()
    )

    # Relationships
    plant: Mapped["Plant | None"] = relationship(  # noqa: F821
        "Plant", back_populates="device", uselist=False
    )
