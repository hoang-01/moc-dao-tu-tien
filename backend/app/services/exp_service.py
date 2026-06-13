"""EXP Service — Thuật toán Tu Vi (EXP) & Cảnh Giới (Rank).

Đây là core logic của hệ thống Gamification Mộc Đạo Tu Tiên.
"""

import logging
from datetime import UTC, datetime

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession


from app.models.breakthrough import BreakthroughEvent
from app.models.config import ExpConfig, PlantType, RankConfig
from app.models.exp_log import ExpLog
from app.models.plant import Plant
from app.services.sse_service import sse_manager

logger = logging.getLogger(__name__)

# --- Hằng số Anti-Spam ---
EXP_CYCLE_SECONDS = 60  # Chu kỳ tiêu chuẩn
GRACE_PERIOD_SECONDS = 5  # Dung sai
MIN_INTERVAL = EXP_CYCLE_SECONDS - GRACE_PERIOD_SECONDS  # 55 giây

# --- Thang đo chất lượng ---
QUALITY_ORDER = ["EXCELLENT", "GOOD", "FAIR", "POOR", "DANGER"]


def classify_sensor_quality(
    value: float,
    ideal_min: float,
    ideal_max: float,
) -> str:
    """Phân loại chất lượng 1 cảm biến dựa trên ngưỡng lý tưởng.

    Logic phân loại:
    - EXCELLENT: Nằm trong khoảng [min, max] (lý tưởng hoàn hảo)
    - GOOD: Lệch ≤10% bên ngoài khoảng lý tưởng
    - FAIR: Lệch ≤25%
    - POOR: Lệch ≤50%
    - DANGER: Lệch >50%
    """
    ideal_range = ideal_max - ideal_min
    if ideal_range <= 0:
        ideal_range = 1.0  # Tránh chia cho 0

    # Nằm trong khoảng lý tưởng
    if ideal_min <= value <= ideal_max:
        return "EXCELLENT"

    # Tính độ lệch ra ngoài khoảng lý tưởng
    if value < ideal_min:
        deviation = (ideal_min - value) / ideal_range
    else:
        deviation = (value - ideal_max) / ideal_range

    if deviation <= 0.10:
        return "GOOD"
    elif deviation <= 0.25:
        return "FAIR"
    elif deviation <= 0.50:
        return "POOR"
    else:
        return "DANGER"


def get_overall_quality(qualities: list[str]) -> str:
    """Tổng hợp chất lượng môi trường = mức XẤU NHẤT.

    Ví dụ: nếu độ ẩm GOOD, ánh sáng POOR → tổng hợp = POOR.
    """
    if not qualities:
        return "FAIR"

    worst_index = 0
    for q in qualities:
        idx = QUALITY_ORDER.index(q) if q in QUALITY_ORDER else 2
        worst_index = max(worst_index, idx)

    return QUALITY_ORDER[worst_index]


async def get_exp_delta(db: AsyncSession, quality_level: str) -> float:
    """Tra bảng ExpConfig để lấy hệ số cộng/trừ Tu Vi."""
    stmt = select(ExpConfig).where(ExpConfig.quality_level == quality_level)
    result = await db.execute(stmt)
    config = result.scalar_one_or_none()
    if config is None:
        # Fallback defaults nếu chưa có config
        defaults = {
            "EXCELLENT": 10.0,
            "GOOD": 5.0,
            "FAIR": 0.0,
            "POOR": -3.0,
            "DANGER": -8.0,
        }
        return defaults.get(quality_level, 0.0)
    return config.exp_delta


async def check_anti_spam(plant: Plant) -> bool:
    """Kiểm tra anti-spam: Có đủ thời gian kể từ lần thưởng cuối không?

    Returns:
        True nếu đủ thời gian → được tính điểm.
        False nếu chưa đủ 55s → bỏ qua.
    """
    if plant.last_exp_reward_at is None:
        return True

    now = datetime.now(UTC)
    last = plant.last_exp_reward_at
    # Đảm bảo last có timezone
    if last.tzinfo is None:
        from datetime import timezone

        last = last.replace(tzinfo=timezone.utc)

    elapsed = (now - last).total_seconds()
    return elapsed >= MIN_INTERVAL


async def determine_rank(db: AsyncSession, total_exp: float) -> RankConfig:
    """Xác định Cảnh Giới phù hợp với mức Tu Vi hiện tại.

    Tìm Cảnh Giới có min_exp cao nhất mà <= total_exp.
    Hỗ trợ 'đột phá vượt cấp' — nếu EXP nhảy qua nhiều mốc.
    """
    stmt = (
        select(RankConfig)
        .where(RankConfig.min_exp <= total_exp)
        .order_by(RankConfig.order.desc())
        .limit(1)
    )
    result = await db.execute(stmt)
    rank = result.scalar_one_or_none()

    if rank is None:
        # Fallback: lấy rank thấp nhất
        stmt = select(RankConfig).order_by(RankConfig.order.asc()).limit(1)
        result = await db.execute(stmt)
        rank = result.scalar_one()

    return rank


async def get_next_rank(db: AsyncSession, current_order: int) -> RankConfig | None:
    """Lấy Cảnh Giới tiếp theo (nếu có)."""
    stmt = (
        select(RankConfig)
        .where(RankConfig.order > current_order)
        .order_by(RankConfig.order.asc())
        .limit(1)
    )
    result = await db.execute(stmt)
    return result.scalar_one_or_none()


async def process_exp(
    db: AsyncSession,
    plant: Plant,
    overall_quality: str,
) -> dict:
    """Xử lý tính điểm Tu Vi cho 1 chu kỳ.

    Returns:
        dict chứa thông tin kết quả:
        - exp_awarded: bool
        - delta: float
        - total_exp: float
        - breakthrough: dict | None (nếu đột phá)
    """
    # 1. Kiểm tra anti-spam
    if not await check_anti_spam(plant):
        logger.debug("Anti-spam: bỏ qua tính điểm cho plant %s", plant.id)
        return {
            "exp_awarded": False,
            "delta": 0,
            "total_exp": plant.total_exp,
            "breakthrough": None,
        }

    # 2. Lấy hệ số cộng/trừ
    delta = await get_exp_delta(db, overall_quality)

    # 3. Cập nhật Tu Vi (không cho âm)
    plant.total_exp = max(0.0, plant.total_exp + delta)
    plant.last_exp_reward_at = datetime.now(UTC)

    # 4. Ghi log
    exp_log = ExpLog(
        plant_id=plant.id,
        delta=delta,
        reason=overall_quality,
        overall_quality=overall_quality,
        total_exp_after=plant.total_exp,
    )
    db.add(exp_log)

    # 5. Kiểm tra đột phá
    # Load current rank để so sánh
    await db.refresh(plant, ["current_rank"])
    old_rank = plant.current_rank
    new_rank = await determine_rank(db, plant.total_exp)

    breakthrough_info = None
    if new_rank.order != old_rank.order:
        plant.current_rank_id = new_rank.id

        # Ghi log đột phá
        breakthrough = BreakthroughEvent(
            plant_id=plant.id,
            from_rank=old_rank.name,
            to_rank=new_rank.name,
            exp_at_breakthrough=plant.total_exp,
        )
        db.add(breakthrough)

        breakthrough_info = {
            "from_rank": old_rank.name,
            "to_rank": new_rank.name,
            "exp": plant.total_exp,
        }

        direction = "thăng cấp" if new_rank.order > old_rank.order else "giáng cấp"
        logger.info(
            "🌟 Cây '%s' %s: %s → %s (Tu Vi: %.1f)",
            plant.name,
            direction,
            old_rank.name,
            new_rank.name,
            plant.total_exp,
        )

    await db.flush()

    # 6. Broadcast SSE
    await sse_manager.broadcast(
        plant.id,
        "exp_update",
        {
            "total_exp": plant.total_exp,
            "delta": delta,
            "quality": overall_quality,
            "rank_name": new_rank.name,
            "rank_order": new_rank.order,
            "breakthrough": breakthrough_info,
        },
    )

    # Gửi cập nhật về thiết bị qua MQTT
    if plant.device:
        try:
            from app.mqtt.client import publish_to_device
            await publish_to_device(
                plant.device.plant_code,
                {
                    "total_exp": int(plant.total_exp),
                    "rank_name": new_rank.name,
                    "is_paired": True
                }
            )
        except Exception as e:
            logger.warning("Không thể publish EXP tới thiết bị qua MQTT: %s", e)

    return {
        "exp_awarded": True,
        "delta": delta,
        "total_exp": plant.total_exp,
        "breakthrough": breakthrough_info,
    }


def classify_sensors_for_plant_type(
    sensor_data: dict[str, float],
    plant_type: PlantType,
) -> dict[str, str]:
    """Phân loại chất lượng tất cả cảm biến dựa trên ngưỡng lý tưởng của loại cây.

    Args:
        sensor_data: Dict {sensor_key: value}
        plant_type: PlantType model chứa ngưỡng lý tưởng

    Returns:
        Dict {sensor_key: quality_level}
    """
    thresholds = {
        "soil_moisture": (plant_type.soil_moisture_min, plant_type.soil_moisture_max),
        "light": (plant_type.light_min, plant_type.light_max),
        "temperature": (plant_type.temperature_min, plant_type.temperature_max),
        "humidity": (plant_type.humidity_min, plant_type.humidity_max),
    }

    qualities = {}
    for key, value in sensor_data.items():
        if key in thresholds:
            min_val, max_val = thresholds[key]
            qualities[key] = classify_sensor_quality(value, min_val, max_val)
        else:
            qualities[key] = "FAIR"  # Sensor không rõ → trung bình

    return qualities
