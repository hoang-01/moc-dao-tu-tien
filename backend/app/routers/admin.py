"""Router Admin — Dashboard thống kê, quản lý thiết bị, cấu hình."""

import logging
from uuid import UUID

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.database import get_db
from app.dependencies import get_admin_user
from app.schemas.admin import (
    ExpConfigUpdateRequest,
    PlantTypeCreateRequest,
    PlantTypeResponse,
    PlantTypeUpdateRequest,
    RankConfigUpdateRequest,
)
from app.schemas.device import DeviceUpdateRequest
from app.services.admin_service import (
    create_plant_type,
    delete_plant_type,
    get_dashboard_stats,
    get_devices_list,
    get_exp_configs,
    get_plant_types,
    get_rank_configs,
    provision_device,
    update_device_status,
    update_exp_configs,
    update_plant_type,
    update_rank_configs,
)

logger = logging.getLogger(__name__)
router = APIRouter(
    prefix="/api/admin", tags=["Admin"], dependencies=[Depends(get_admin_user)]
)


# --- Dashboard ---
@router.get("/dashboard")
async def admin_dashboard(
    db: AsyncSession = Depends(get_db),
) -> dict:
    """Thống kê tổng quan hệ thống cho Admin."""
    return await get_dashboard_stats(db)


# --- Device Management ---
@router.get("/devices")
async def list_devices(
    db: AsyncSession = Depends(get_db),
) -> list[dict]:
    """Danh sách tất cả thiết bị IoT với trạng thái."""
    return await get_devices_list(db)


@router.post("/devices")
async def create_device(
    db: AsyncSession = Depends(get_db),
) -> dict:
    """Tạo thiết bị mới (Provisioning).

    Hệ thống tự sinh Plant Code + Verify Code.
    Verify Code chỉ được hiển thị 1 lần duy nhất.
    """
    return await provision_device(db)


@router.put("/devices/{device_id}")
async def update_device(
    device_id: UUID,
    body: DeviceUpdateRequest,
    db: AsyncSession = Depends(get_db),
) -> dict:
    """Vô hiệu hóa/kích hoạt thiết bị."""
    try:
        device = await update_device_status(db, str(device_id), body.is_active)
        return {
            "status": "success",
            "plant_code": device.plant_code,
            "is_active": device.is_active,
        }
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=str(e),
        ) from e


# --- Plant Type Management ---
@router.get("/plant-types")
async def list_plant_types(
    db: AsyncSession = Depends(get_db),
) -> list[PlantTypeResponse]:
    """Danh sách tất cả loại cây."""
    types = await get_plant_types(db)
    return [PlantTypeResponse.model_validate(t) for t in types]


@router.post("/plant-types", status_code=status.HTTP_201_CREATED)
async def create_plant_type_endpoint(
    body: PlantTypeCreateRequest,
    db: AsyncSession = Depends(get_db),
) -> PlantTypeResponse:
    """Thêm loại cây mới."""
    try:
        pt = await create_plant_type(db, body.model_dump())
        return PlantTypeResponse.model_validate(pt)
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e),
        ) from e


@router.put("/plant-types/{type_id}")
async def update_plant_type_endpoint(
    type_id: UUID,
    body: PlantTypeUpdateRequest,
    db: AsyncSession = Depends(get_db),
) -> PlantTypeResponse:
    """Sửa loại cây & ngưỡng lý tưởng."""
    try:
        pt = await update_plant_type(
            db, str(type_id), body.model_dump(exclude_unset=True)
        )
        return PlantTypeResponse.model_validate(pt)
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=str(e),
        ) from e


@router.delete("/plant-types/{type_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_plant_type_endpoint(
    type_id: UUID,
    db: AsyncSession = Depends(get_db),
) -> None:
    """Xóa loại cây."""
    try:
        await delete_plant_type(db, str(type_id))
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e),
        ) from e


# --- EXP Config ---
@router.get("/exp-config")
async def get_exp_config(
    db: AsyncSession = Depends(get_db),
) -> list[dict]:
    """Lấy cấu hình hệ số Tu Vi."""
    configs = await get_exp_configs(db)
    return [
        {
            "id": str(c.id),
            "quality_level": c.quality_level,
            "exp_delta": c.exp_delta,
            "description": c.description,
        }
        for c in configs
    ]


@router.put("/exp-config")
async def update_exp_config(
    body: ExpConfigUpdateRequest,
    db: AsyncSession = Depends(get_db),
) -> dict:
    """Cập nhật hệ số cộng/trừ Tu Vi."""
    configs = await update_exp_configs(db, [c.model_dump() for c in body.configs])
    return {
        "status": "success",
        "message": f"Đã cập nhật {len(configs)} cấu hình Tu Vi",
    }


# --- Rank Config ---
@router.get("/rank-config")
async def get_rank_config(
    db: AsyncSession = Depends(get_db),
) -> list[dict]:
    """Lấy danh sách Cảnh Giới."""
    ranks = await get_rank_configs(db)
    return [
        {
            "id": str(r.id),
            "order": r.order,
            "name": r.name,
            "min_exp": r.min_exp,
        }
        for r in ranks
    ]


@router.put("/rank-config")
async def update_rank_config(
    body: RankConfigUpdateRequest,
    db: AsyncSession = Depends(get_db),
) -> dict:
    """Cập nhật mốc Cảnh Giới."""
    ranks = await update_rank_configs(db, [r.model_dump() for r in body.ranks])
    return {
        "status": "success",
        "message": f"Đã cập nhật {len(ranks)} Cảnh Giới",
    }
