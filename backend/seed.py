"""Seed Script — Tạo dữ liệu mặc định cho hệ thống Mộc Đạo Tu Tiên.

Chạy: uv run python seed.py

Tạo:
- 8 Cảnh Giới mặc định (Phàm Mộc → Độ Kiếp)
- 5 Cấu hình hệ số Tu Vi (EXCELLENT → DANGER)
- 3 Loại cây mẫu (Kim Tiền, Lưỡi Hổ, Trầu Bà)
- 1 Admin user (nếu ADMIN_EMAIL được set)
"""

import asyncio
import logging
import os

from sqlalchemy import select

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


async def seed():
    """Tạo dữ liệu mặc định."""
    from app.database import async_session_factory
    from app.models.config import ExpConfig, PlantType, RankConfig
    from app.models.user import User

    async with async_session_factory() as db:
        # --- Cảnh Giới mặc định ---
        ranks_data = [
            {"order": 1, "name": "Phàm Mộc", "min_exp": 0},
            {"order": 2, "name": "Luyện Khí", "min_exp": 100},
            {"order": 3, "name": "Trúc Cơ", "min_exp": 500},
            {"order": 4, "name": "Kim Đan", "min_exp": 1500},
            {"order": 5, "name": "Nguyên Anh", "min_exp": 4000},
            {"order": 6, "name": "Hóa Thần", "min_exp": 8000},
            {"order": 7, "name": "Đại Thừa", "min_exp": 15000},
            {"order": 8, "name": "Độ Kiếp", "min_exp": 30000},
        ]

        for r in ranks_data:
            existing = await db.execute(
                select(RankConfig).where(RankConfig.order == r["order"])
            )
            if existing.scalar_one_or_none() is None:
                db.add(RankConfig(**r))
                logger.info("  ⭐ Cảnh Giới: %s (Tu Vi ≥ %d)", r["name"], r["min_exp"])

        # --- Hệ số Tu Vi mặc định ---
        exp_data = [
            {
                "quality_level": "EXCELLENT",
                "exp_delta": 10.0,
                "description": "Môi trường lý tưởng hoàn hảo",
            },
            {
                "quality_level": "GOOD",
                "exp_delta": 5.0,
                "description": "Môi trường tốt, lệch nhẹ so với lý tưởng",
            },
            {
                "quality_level": "FAIR",
                "exp_delta": 0.0,
                "description": "Môi trường trung bình, không ảnh hưởng Tu Vi",
            },
            {
                "quality_level": "POOR",
                "exp_delta": -3.0,
                "description": "Môi trường kém, cây bị tổn hao Tu Vi",
            },
            {
                "quality_level": "DANGER",
                "exp_delta": -8.0,
                "description": "Môi trường nguy hiểm, cây đang 'tẩu hỏa nhập ma'",
            },
        ]

        for e in exp_data:
            existing = await db.execute(
                select(ExpConfig).where(ExpConfig.quality_level == e["quality_level"])
            )
            if existing.scalar_one_or_none() is None:
                db.add(ExpConfig(**e))
                logger.info(
                    "  ⚙️ Hệ số Tu Vi: %s → %+.1f",
                    e["quality_level"],
                    e["exp_delta"],
                )

        # --- Loại cây mẫu ---
        plants_data = [
            {
                "name": "Kim Tiền",
                "description": "Cây Kim Tiền (Zamioculcas zamiifolia) — cây phong thủy dễ chăm sóc, ưa bóng râm.",
                "soil_moisture_min": 30.0,
                "soil_moisture_max": 60.0,
                "light_min": 500.0,
                "light_max": 5000.0,
                "temperature_min": 18.0,
                "temperature_max": 30.0,
                "humidity_min": 40.0,
                "humidity_max": 70.0,
            },
            {
                "name": "Lưỡi Hổ",
                "description": "Cây Lưỡi Hổ (Sansevieria) — cực kỳ dễ sống, lọc không khí tốt.",
                "soil_moisture_min": 20.0,
                "soil_moisture_max": 50.0,
                "light_min": 1000.0,
                "light_max": 15000.0,
                "temperature_min": 15.0,
                "temperature_max": 35.0,
                "humidity_min": 30.0,
                "humidity_max": 70.0,
            },
            {
                "name": "Trầu Bà",
                "description": "Cây Trầu Bà (Epipremnum aureum) — dây leo phong thủy, ưa ẩm.",
                "soil_moisture_min": 40.0,
                "soil_moisture_max": 70.0,
                "light_min": 1000.0,
                "light_max": 8000.0,
                "temperature_min": 20.0,
                "temperature_max": 32.0,
                "humidity_min": 50.0,
                "humidity_max": 80.0,
            },
        ]

        for p in plants_data:
            existing = await db.execute(
                select(PlantType).where(PlantType.name == p["name"])
            )
            if existing.scalar_one_or_none() is None:
                db.add(PlantType(**p))
                logger.info("  🌿 Loại cây: %s", p["name"])

        # --- Admin user (optional) ---
        admin_email = os.environ.get("ADMIN_EMAIL")
        if admin_email:
            existing = await db.execute(select(User).where(User.email == admin_email))
            user = existing.scalar_one_or_none()
            if user:
                user.role = "admin"
                logger.info("  👑 Đã nâng quyền Admin: %s", admin_email)
            else:
                logger.info(
                    "  ℹ️ User %s chưa tồn tại, sẽ được set admin khi đăng nhập lần đầu.",
                    admin_email,
                )

        await db.commit()
        logger.info("✅ Seed data hoàn tất!")


if __name__ == "__main__":
    logger.info("🌱 Bắt đầu seed dữ liệu mặc định...")
    asyncio.run(seed())
