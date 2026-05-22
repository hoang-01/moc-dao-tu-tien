"""Admin Service — Thống kê hệ thống & quản lý cấu hình."""

import logging
import secrets
import string
from datetime import UTC, datetime, timedelta

import bcrypt
from sqlalchemy import func, select
from sqlalchemy.orm import selectinload
from sqlalchemy.ext.asyncio import AsyncSession

from app.models.config import ExpConfig, PlantType, RankConfig
from app.models.device import Device
from app.models.plant import Plant
from app.models.user import User

logger = logging.getLogger(__name__)

# Thời gian coi thiết bị là offline
DEVICE_OFFLINE_SECONDS = 120


def generate_plant_code(length: int = 8) -> str:
    """Sinh Plant Code ngẫu nhiên (8 ký tự chữ HOA + số)."""
    alphabet = string.ascii_uppercase + string.digits
    return "".join(secrets.choice(alphabet) for _ in range(length))


def generate_verify_code(length: int = 6) -> str:
    """Sinh Verify Code ngẫu nhiên (6 chữ số)."""
    return "".join(secrets.choice(string.digits) for _ in range(length))


async def provision_device(db: AsyncSession) -> dict:
    """Tạo thiết bị mới: sinh Plant Code + Verify Code.

    Returns:
        dict chứa device info + verify_code gốc (chỉ hiển thị 1 lần).
    """
    # Sinh plant_code duy nhất
    for _ in range(10):
        plant_code = generate_plant_code()
        stmt = select(Device).where(Device.plant_code == plant_code)
        result = await db.execute(stmt)
        if result.scalar_one_or_none() is None:
            break
    else:
        raise ValueError("Không thể sinh Plant Code duy nhất sau 10 lần thử")

    # Sinh verify_code và hash
    verify_code = generate_verify_code()
    verify_hash = bcrypt.hashpw(
        verify_code.encode("utf-8"),
        bcrypt.gensalt(),
    ).decode("utf-8")

    device = Device(
        plant_code=plant_code,
        verify_hash=verify_hash,
    )
    db.add(device)
    await db.flush()

    logger.info("📟 Thiết bị mới: Plant Code = %s", plant_code)

    return {
        "id": str(device.id),
        "plant_code": plant_code,
        "verify_code": verify_code,  # Chỉ trả về 1 lần!
        "created_at": device.created_at.isoformat(),
    }


async def get_dashboard_stats(db: AsyncSession) -> dict:
    """Lấy thống kê tổng quan hệ thống cho Admin Dashboard."""
    now = datetime.now(UTC)
    offline_threshold = now - timedelta(seconds=DEVICE_OFFLINE_SECONDS)

    # Tổng users
    result = await db.execute(select(func.count(User.id)))
    total_users = result.scalar() or 0

    # Tổng devices
    result = await db.execute(select(func.count(Device.id)))
    total_devices = result.scalar() or 0

    # Devices online (last_seen_at gần đây)
    result = await db.execute(
        select(func.count(Device.id)).where(
            Device.last_seen_at >= offline_threshold,
            Device.is_active.is_(True),
        )
    )
    devices_online = result.scalar() or 0
    devices_offline = total_devices - devices_online

    # Tổng cây đã liên kết
    result = await db.execute(select(func.count(Plant.id)))
    total_plants = result.scalar() or 0

    # Phân bố Cảnh Giới
    stmt = (
        select(RankConfig.name, func.count(Plant.id))
        .outerjoin(Plant, Plant.current_rank_id == RankConfig.id)
        .group_by(RankConfig.name, RankConfig.order)
        .order_by(RankConfig.order)
    )
    result = await db.execute(stmt)
    rank_distribution = [
        {"rank_name": row[0], "count": row[1] or 0} for row in result.all()
    ]

    # Người dùng mới 7 ngày gần nhất
    new_users_daily = []
    for i in range(7):
        day = (now - timedelta(days=i)).date()
        day_start = datetime(day.year, day.month, day.day, tzinfo=UTC)
        day_end = day_start + timedelta(days=1)
        result = await db.execute(
            select(func.count(User.id)).where(
                User.created_at >= day_start,
                User.created_at < day_end,
            )
        )
        count = result.scalar() or 0
        new_users_daily.append({"date": day.isoformat(), "count": count})

    new_users_daily.reverse()  # Sắp xếp từ cũ → mới

    return {
        "total_users": total_users,
        "total_devices": total_devices,
        "devices_online": devices_online,
        "devices_offline": devices_offline,
        "total_plants_paired": total_plants,
        "rank_distribution": rank_distribution,
        "new_users_daily": new_users_daily,
    }


async def get_devices_list(db: AsyncSession) -> list[dict]:
    """Lấy danh sách tất cả thiết bị với trạng thái."""

    stmt = select(Device).order_by(Device.created_at.desc())
    result = await db.execute(stmt)
    devices = result.scalars().all()

    device_list = []
    for d in devices:
        # Tìm plant liên kết (nếu có)
        paired_user_email = None
        paired_plant_name = None
        if d.is_paired:
            stmt = (
                select(Plant)
                .where(Plant.device_id == d.id)
                .options(selectinload(Plant.user))
            )
            result = await db.execute(stmt)
            plant = result.scalar_one_or_none()
            if plant:
                paired_plant_name = plant.name
                paired_user_email = plant.user.email if plant.user else None

        device_list.append(
            {
                "id": str(d.id),
                "plant_code": d.plant_code,
                "is_paired": d.is_paired,
                "is_active": d.is_active,
                "last_seen_at": d.last_seen_at.isoformat() if d.last_seen_at else None,
                "paired_user_email": paired_user_email,
                "paired_plant_name": paired_plant_name,
                "created_at": d.created_at.isoformat(),
            }
        )

    return device_list


async def update_device_status(
    db: AsyncSession,
    device_id: str,
    is_active: bool,
) -> Device:
    """Vô hiệu hóa / kích hoạt thiết bị."""
    from uuid import UUID as PyUUID

    stmt = select(Device).where(Device.id == PyUUID(device_id))
    result = await db.execute(stmt)
    device = result.scalar_one_or_none()

    if device is None:
        raise ValueError("Thiết bị không tồn tại")

    device.is_active = is_active
    await db.flush()

    status = "kích hoạt" if is_active else "vô hiệu hóa"
    logger.info("📟 Thiết bị %s đã được %s", device.plant_code, status)
    return device


# --- Plant Type CRUD ---


async def get_plant_types(db: AsyncSession) -> list[PlantType]:
    """Lấy danh sách tất cả loại cây."""
    stmt = select(PlantType).order_by(PlantType.name)
    result = await db.execute(stmt)
    return list(result.scalars().all())


async def create_plant_type(db: AsyncSession, data: dict) -> PlantType:
    """Tạo loại cây mới."""
    # Kiểm tra tên trùng
    stmt = select(PlantType).where(PlantType.name == data["name"])
    result = await db.execute(stmt)
    if result.scalar_one_or_none() is not None:
        raise ValueError(f"Loại cây '{data['name']}' đã tồn tại")

    plant_type = PlantType(**data)
    db.add(plant_type)
    await db.flush()
    logger.info("🌿 Loại cây mới: %s", plant_type.name)
    return plant_type


async def update_plant_type(db: AsyncSession, type_id: str, data: dict) -> PlantType:
    """Cập nhật loại cây."""
    from uuid import UUID as PyUUID

    stmt = select(PlantType).where(PlantType.id == PyUUID(type_id))
    result = await db.execute(stmt)
    plant_type = result.scalar_one_or_none()

    if plant_type is None:
        raise ValueError("Loại cây không tồn tại")

    for key, value in data.items():
        if value is not None:
            setattr(plant_type, key, value)

    await db.flush()
    return plant_type


async def delete_plant_type(db: AsyncSession, type_id: str) -> None:
    """Xóa loại cây."""
    from uuid import UUID as PyUUID

    stmt = select(PlantType).where(PlantType.id == PyUUID(type_id))
    result = await db.execute(stmt)
    plant_type = result.scalar_one_or_none()

    if plant_type is None:
        raise ValueError("Loại cây không tồn tại")

    # Kiểm tra có cây nào đang dùng loại này không
    stmt = select(func.count(Plant.id)).where(Plant.plant_type_id == plant_type.id)
    result = await db.execute(stmt)
    count = result.scalar() or 0
    if count > 0:
        raise ValueError(f"Không thể xóa: có {count} cây đang sử dụng loại cây này")

    await db.delete(plant_type)
    await db.flush()
    logger.info("🗑️ Đã xóa loại cây: %s", plant_type.name)


# --- EXP Config ---


async def get_exp_configs(db: AsyncSession) -> list[ExpConfig]:
    """Lấy danh sách cấu hình hệ số Tu Vi."""
    stmt = select(ExpConfig).order_by(ExpConfig.quality_level)
    result = await db.execute(stmt)
    return list(result.scalars().all())


async def update_exp_configs(db: AsyncSession, configs: list[dict]) -> list[ExpConfig]:
    """Cập nhật (upsert) cấu hình hệ số Tu Vi."""
    results = []
    for c in configs:
        stmt = select(ExpConfig).where(ExpConfig.quality_level == c["quality_level"])
        result = await db.execute(stmt)
        exp_config = result.scalar_one_or_none()

        if exp_config:
            exp_config.exp_delta = c["exp_delta"]
            exp_config.description = c.get("description")
        else:
            exp_config = ExpConfig(
                quality_level=c["quality_level"],
                exp_delta=c["exp_delta"],
                description=c.get("description"),
            )
            db.add(exp_config)

        results.append(exp_config)

    await db.flush()
    logger.info("⚙️ Cấu hình Tu Vi đã được cập nhật")
    return results


# --- Rank Config ---


async def get_rank_configs(db: AsyncSession) -> list[RankConfig]:
    """Lấy danh sách Cảnh Giới."""
    stmt = select(RankConfig).order_by(RankConfig.order)
    result = await db.execute(stmt)
    return list(result.scalars().all())


async def update_rank_configs(db: AsyncSession, ranks: list[dict]) -> list[RankConfig]:
    """Cập nhật (replace all) danh sách Cảnh Giới.

    Xóa tất cả rank cũ và tạo lại từ đầu.
    Cảnh Giới hiện tại của các cây sẽ được cập nhật lại.
    """
    # Xóa tất cả rank configs cũ — trước đó cần unlink plants
    # Cách an toàn: update từng rank theo order
    for r in ranks:
        stmt = select(RankConfig).where(RankConfig.order == r["order"])
        result = await db.execute(stmt)
        rank = result.scalar_one_or_none()

        if rank:
            rank.name = r["name"]
            rank.min_exp = r["min_exp"]
        else:
            rank = RankConfig(
                order=r["order"],
                name=r["name"],
                min_exp=r["min_exp"],
            )
            db.add(rank)

    await db.flush()
    logger.info("⚙️ Cảnh Giới đã được cập nhật (%d cấp)", len(ranks))

    # Trả về danh sách mới
    return await get_rank_configs(db)
