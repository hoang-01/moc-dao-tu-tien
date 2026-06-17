"""Script Python test để tạo thiết bị thử nghiệm (plant_code = 'TESTDEV1') trong Database."""

import asyncio
import logging
import sys
import os

# Thêm thư mục gốc của backend vào sys.path để import app
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pyrefly: ignore [missing-import]
from sqlalchemy import select
from app.database import async_session_factory
from app.models.user import User
from app.models.device import Device
from app.models.plant import Plant
from app.models.config import PlantType, RankConfig

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("create_test_device")


async def create_test_device():
    async with async_session_factory() as db:
        # 1. Tìm hoặc tạo User test
        test_email = "test_user@example.com"
        user_stmt = select(User).where(User.email == test_email)
        user_res = await db.execute(user_stmt)
        user = user_res.scalar_one_or_none()

        if not user:
            logger.info("Test user not found, creating new one...")
            user = User(
                google_id="test_google_id_123456",
                email=test_email,
                display_name="Đạo Hữu Thử Nghiệm",
                role="user",
            )
            db.add(user)
            await db.flush()  # Để sinh user.id
            logger.info(f"Created test user with ID: {user.id}")
        else:
            logger.info(f"Found existing test user with ID: {user.id}")

        # 2. Lấy và cập nhật PlantType "Kim Tiền"
        pt_stmt = select(PlantType).where(PlantType.name == "Kim Tiền")
        pt_res = await db.execute(pt_stmt)
        plant_type = pt_res.scalar_one_or_none()

        if not plant_type:
            logger.error(
                "PlantType 'Kim Tiền' not found in database. Please run python seed.py first."
            )
            return

        logger.info(
            f"Found PlantType 'Kim Tiền' with ID: {plant_type.id}. Updating ideal thresholds for real-world logs..."
        )
        plant_type.temperature_min = 20.0
        plant_type.temperature_max = 32.0
        plant_type.humidity_min = 40.0
        plant_type.humidity_max = 70.0
        plant_type.soil_moisture_min = 80.0
        plant_type.soil_moisture_max = 100.0
        plant_type.light_min = -5.0
        plant_type.light_max = 10000.0
        await db.flush()

        # 3. Lấy RankConfig "Phàm Mộc" (order = 1)
        rank_stmt = select(RankConfig).where(RankConfig.order == 1)
        rank_res = await db.execute(rank_stmt)
        rank = rank_res.scalar_one_or_none()

        if not rank:
            logger.error(
                "RankConfig for order 1 not found in database. Please run python seed.py first."
            )
            return

        logger.info(f"Found RankConfig 'Phàm Mộc' with ID: {rank.id}")

        # 4. Tìm hoặc tạo Device "TESTDEV1"
        plant_code = "TESTDEV1"
        # Bcrypt hash cho verify_code = "123456"
        verify_hash = "$2b$12$N9qo8uLOqpGC12Z0LFv2.uWJtPZqP8pB7jYJv3WUXl6H7Hh2G3eKi"

        import bcrypt

        is_match = bcrypt.checkpw("123456".encode("utf-8"), verify_hash.encode("utf-8"))
        logger.info(f"Initial verify_hash matches '123456': {is_match}")
        if not is_match:
            logger.info(
                "Hardcoded verify_hash does not match '123456'. Generating a correct hash..."
            )
            verify_hash = bcrypt.hashpw(
                "123456".encode("utf-8"), bcrypt.gensalt()
            ).decode("utf-8")
            logger.info(f"Generated correct hash: {verify_hash}")

        device_stmt = select(Device).where(Device.plant_code == plant_code)
        device_res = await db.execute(device_stmt)
        device = device_res.scalar_one_or_none()

        if not device:
            logger.info("Device 'TESTDEV1' not found, creating...")
            device = Device(
                plant_code=plant_code,
                verify_hash=verify_hash,
                is_paired=True,
                is_active=True,
            )
            db.add(device)
            await db.flush()  # Để sinh device.id
            logger.info(f"Created device with ID: {device.id}")
        else:
            logger.info(
                f"Found existing device with ID: {device.id}. Updating state..."
            )
            device.verify_hash = verify_hash
            device.is_paired = True
            device.is_active = True
            await db.flush()

        # 5. Tìm hoặc tạo Plant liên kết
        plant_stmt = select(Plant).where(Plant.device_id == device.id)
        plant_res = await db.execute(plant_stmt)
        plant = plant_res.scalar_one_or_none()

        if not plant:
            logger.info("Plant link not found, creating plant...")
            plant = Plant(
                user_id=user.id,
                device_id=device.id,
                plant_type_id=plant_type.id,
                name="Chậu Kim Tiền Đột Biến",
                total_exp=0.0,
                current_rank_id=rank.id,
                current_overall_quality="EXCELLENT",
            )
            db.add(plant)
            logger.info("Created plant successfully.")
        else:
            logger.info(f"Plant already exists with ID: {plant.id}")
            plant.name = "Chậu Kim Tiền Đột Biến"
            plant.user_id = user.id
            plant.plant_type_id = plant_type.id
            plant.current_rank_id = rank.id
            plant.current_overall_quality = "EXCELLENT"
            logger.info("Updated plant link successfully.")

        await db.commit()
        logger.info("Database committed successfully.")


if __name__ == "__main__":
    asyncio.run(create_test_device())
