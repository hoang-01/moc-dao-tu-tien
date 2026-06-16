"""Router Devices — Telemetry endpoint cho thiết bị IoT."""

import logging

from fastapi import APIRouter, Depends, Header, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.database import get_db
from app.schemas.telemetry import TelemetryPayload, TelemetryResponse
from app.services.telemetry_service import process_telemetry

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/api/devices", tags=["Devices"])


@router.post("/{plant_code}/telemetry", response_model=TelemetryResponse)
async def receive_telemetry(
    plant_code: str,
    body: TelemetryPayload,
    x_plant_code: str = Header(..., description="Plant Code để xác thực thiết bị"),
    db: AsyncSession = Depends(get_db),
) -> TelemetryResponse:
    """Nhận dữ liệu cảm biến từ thiết bị IoT (REST fallback).

    Primary: MQTT. Endpoint này là fallback khi MQTT không khả dụng.
    Auth: X-Plant-Code header phải khớp với plant_code trong URL.
    """
    # Xác thực: header phải khớp path
    if x_plant_code != plant_code:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="X-Plant-Code không khớp",
        )

    try:
        result = await process_telemetry(db, plant_code, body.sensors)
        return TelemetryResponse(
            status=result["status"],
            exp_awarded=result.get("exp_awarded", False),
            total_exp=result.get("total_exp", 0.0),
            rank_name=result.get("rank_name", "Phàm Mộc"),
            message=result.get("message"),
        )
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e),
        ) from e
