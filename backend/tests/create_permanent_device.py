#!/usr/bin/env python3
"""Tạo Thiết Bị và Cây Trồng Cố Định cho Bot Giả Lập.

Script này sẽ tạo một tài khoản người dùng kiểm thử, một thiết bị mẫu YHB869Y5 (được ghép đôi)
và cây trồng mẫu (loại Kim Tiền) trực tiếp trong Database để bot giả lập có thể chạy thành công lập tức.
"""

import asyncio
import sys
import bcrypt
from sqlalchemy import select, delete

from app.database import async_session_factory
from app.models.user import User
from app.models.device import Device
from app.models.plant import Plant
from app.models.config import PlantType, RankConfig
from app.models.sensor_reading import SensorReading
from app.models.exp_log import ExpLog
from app.models.breakthrough import BreakthroughEvent


async def create_permanent_device():
    print("⚙️ Đang thiết lập thiết bị và cây trồng cố định...")
    email = "test-bot-user@moctu.com"
    plant_code = "YHB869Y5"
    verify_code = "123456"

    async with async_session_factory() as db:
        # 1. Tìm hoặc Tạo người dùng kiểm thử
        stmt = select(User).where(User.email == email)
        res = await db.execute(stmt)
        user = res.scalar_one_or_none()

        if user is None:
            user = User(
                google_id="mock-google-id-bot-user",
                email=email,
                display_name="Đạo Hữu Giả Lập",
                role="user",
            )
            db.add(user)
            await db.flush()
            print(f"  └ ✅ Tạo User kiểm thử mới: {email}")
        else:
            print(f"  └ ✅ Dùng lại User hiện tại: {email}")

        # 2. Xóa các bản ghi cũ của Plant/Device nếu trùng mã
        stmt = select(Device).where(Device.plant_code == plant_code)
        res = await db.execute(stmt)
        existing_device = res.scalar_one_or_none()

        if existing_device:
            # Xóa các liên kết trước
            stmt = select(Plant).where(Plant.device_id == existing_device.id)
            res = await db.execute(stmt)
            existing_plant = res.scalar_one_or_none()
            if existing_plant:
                await db.execute(delete(SensorReading).where(SensorReading.plant_id == existing_plant.id))
                await db.execute(delete(ExpLog).where(ExpLog.plant_id == existing_plant.id))
                await db.execute(delete(BreakthroughEvent).where(BreakthroughEvent.plant_id == existing_plant.id))
                await db.execute(delete(Plant).where(Plant.id == existing_plant.id))
            await db.execute(delete(Device).where(Device.id == existing_device.id))
            await db.flush()
            print(f"  └ 🧹 Đã dọn dẹp thiết bị cũ trùng mã {plant_code}")

        # 3. Tạo thiết bị mới đã kích hoạt và đánh dấu là paired
        verify_hash = bcrypt.hashpw(
            verify_code.encode("utf-8"),
            bcrypt.gensalt(),
        ).decode("utf-8")

        device = Device(
            plant_code=plant_code,
            verify_hash=verify_hash,
            is_paired=True,
            is_active=True,
        )
        db.add(device)
        await db.flush()
        print(f"  └ ✅ Tạo thiết bị mới: {plant_code} (Verify Code: {verify_code})")

        # 4. Lấy mẫu loại cây trồng (Kim Tiền)
        stmt = select(PlantType).where(PlantType.name == "Kim Tiền")
        res = await db.execute(stmt)
        plant_type = res.scalar_one_or_none()
        if not plant_type:
            # Nếu không tìm thấy, lấy bừa loại cây đầu tiên
            stmt = select(PlantType).limit(1)
            res = await db.execute(stmt)
            plant_type = res.scalar_one_or_none()

        if not plant_type:
            print("❌ Lỗi: Không tìm thấy loại cây nào trong DB! Hãy chạy seed.py trước.")
            sys.exit(1)

        # 5. Lấy Cảnh Giới mặc định (Phàm Mộc)
        stmt = select(RankConfig).order_by(RankConfig.order.asc()).limit(1)
        res = await db.execute(stmt)
        default_rank = res.scalar_one_or_none()
        if not default_rank:
            print("❌ Lỗi: Không tìm thấy cấu hình Cảnh Giới trong DB! Hãy chạy seed.py trước.")
            sys.exit(1)

        # 6. Tạo chậu cây và liên kết với User + Device
        plant = Plant(
            user_id=user.id,
            device_id=device.id,
            plant_type_id=plant_type.id,
            name="Kim Tiền Đắc Đạo",
            total_exp=0.0,
            current_rank_id=default_rank.id,
        )
        db.add(plant)
        await db.commit()
        print(f"  └ ✅ Tạo cây trồng và liên kết thành công: '{plant.name}' ({plant_type.name})")
        print("\n🎉 THIẾT LẬP THÀNH CÔNG! Đạo hữu có thể khởi chạy bot giả lập ngay.")


if __name__ == "__main__":
    asyncio.run(create_permanent_device())
