"""Router Devices — Telemetry endpoint cho thiết bị IoT."""

import logging
from uuid import UUID

import bcrypt
from fastapi import APIRouter, Depends, Header, HTTPException, status
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy.orm import selectinload

from app.database import get_db
from app.models.device import Device
from app.models.plant import Plant
from app.schemas.device import DeviceAuthPayload, DeviceAuthResponse
from app.schemas.telemetry import TelemetryPayload, TelemetryResponse
from app.services.auth_service import create_access_token
from app.services.telemetry_service import process_telemetry

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/api/devices", tags=["Devices"])


@router.post("/{plant_code}/auth", response_model=DeviceAuthResponse)
async def auth_device(
    plant_code: str,
    payload: DeviceAuthPayload,
    db: AsyncSession = Depends(get_db),
) -> DeviceAuthResponse:
    """Xác thực thiết bị IoT qua plant_code và verify_code.

    Trả về JWT token và các ngưỡng lý tưởng của cây đang liên kết.
    """
    stmt = (
        select(Device)
        .options(
            selectinload(Device.plant).selectinload(Plant.plant_type),
            selectinload(Device.plant).selectinload(Plant.current_rank),
        )
        .where(Device.plant_code == plant_code)
    )
    result = await db.execute(stmt)
    device = result.scalar_one_or_none()

    if device is None:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Thiết bị không tồn tại",
        )

    if not device.is_active:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Thiết bị đã bị vô hiệu hóa",
        )

    # Kiểm tra verify_code
    try:
        is_valid = bcrypt.checkpw(
            payload.verify_code.encode("utf-8"),
            device.verify_hash.encode("utf-8"),
        )
    except Exception as e:
        logger.error("Lỗi kiểm tra verify_hash: %s", e)
        is_valid = False

    if not is_valid:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Verify code không chính xác",
        )

    # Tạo JWT token
    if device.plant:
        user_id = device.plant.user_id
    else:
        user_id = UUID("00000000-0000-0000-0000-000000000000")

    token = create_access_token(user_id=user_id, role="device")

    # Đóng gói thresholds
    thresholds = None
    if device.plant and device.plant.plant_type:
        pt = device.plant.plant_type
        thresholds = {
            "temperature_min": pt.temperature_min,
            "temperature_max": pt.temperature_max,
            "humidity_min": pt.humidity_min,
            "humidity_max": pt.humidity_max,
            "soil_moisture_min": pt.soil_moisture_min,
            "soil_moisture_max": pt.soil_moisture_max,
            "light_min": pt.light_min,
            "light_max": pt.light_max,
        }

    from app.scheduler import get_next_reward_seconds

    next_sec = get_next_reward_seconds()

    total_exp = 0.0
    rank_name = "Chưa rõ"
    if device.plant:
        total_exp = device.plant.total_exp
        if device.plant.current_rank:
            rank_name = device.plant.current_rank.name

    return DeviceAuthResponse(
        token=token,
        thresholds=thresholds,
        next_reward_in_seconds=next_sec,
        total_exp=total_exp,
        rank_name=rank_name,
    )


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
