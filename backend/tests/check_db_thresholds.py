import asyncio
import logging
import sys
import os

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from sqlalchemy import select
from sqlalchemy.orm import selectinload
from app.database import async_session_factory
from app.models.device import Device
from app.models.plant import Plant

logging.basicConfig(level=logging.ERROR)


async def check_db():
    async with async_session_factory() as db:
        plant_code = "TESTDEV1"
        stmt = (
            select(Device)
            .where(Device.plant_code == plant_code)
            .options(selectinload(Device.plant).selectinload(Plant.plant_type))
        )
        res = await db.execute(stmt)
        device = res.scalar_one_or_none()

        if not device:
            print(f"❌ KHÔNG TÌM THẤY Thiết bị '{plant_code}' trong Database!")
            return

        print("==================================================")
        print(f"📡 THÔNG TIN THIẾT BỊ: {plant_code}")
        print(f"   - ID: {device.id}")
        print(f"   - Is Active: {device.is_active}")
        print(f"   - Is Paired: {device.is_paired}")
        print(f"   - Last Seen: {device.last_seen_at}")
        print("==================================================")

        if not device.plant:
            print("❌ Thiết bị chưa được liên kết với bất kỳ Chậu Cây (Plant) nào!")
            return

        plant = device.plant
        print(f"🌿 THÔNG TIN CHẬU CÂY: '{plant.name}'")
        print(f"   - EXP hiện tại: {plant.total_exp}")
        print(f"   - Cảnh giới ID: {plant.current_rank_id}")
        print(f"   - Chất lượng chung: {plant.current_overall_quality}")
        print("==================================================")

        if not plant.plant_type:
            print("❌ Chậu cây chưa liên kết với Loại Cây (PlantType) nào!")
            return

        pt = plant.plant_type
        print(f"📖 THƯỜNG TRỰC CẤU HÌNH NGƯỠNG ({pt.name}):")
        print(
            f"   - Nhiệt độ lý tưởng  : {pt.temperature_min} C  đến  {pt.temperature_max} C"
        )
        print(f"   - Độ ẩm KK lý tưởng  : {pt.humidity_min}%  đến  {pt.humidity_max}%")
        print(
            f"   - Độ ẩm đất lý tưởng : {pt.soil_moisture_min}%  đến  {pt.soil_moisture_max}%"
        )
        print(f"   - Ánh sáng lý tưởng  : {pt.light_min} lux  đến  {pt.light_max} lux")
        print("==================================================")


if __name__ == "__main__":
    asyncio.run(check_db())
