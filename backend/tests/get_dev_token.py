#!/usr/bin/env python3
"""Công Cụ Sinh JWT Token để Kiểm Thử Phát Triển."""

import asyncio
import argparse
from uuid import uuid4
from sqlalchemy import select

from app.database import async_session_factory
from app.models.user import User
from app.services.auth_service import create_access_token, create_refresh_token


async def get_or_create_dev_user(email: str, role: str) -> User:
    async with async_session_factory() as db:
        stmt = select(User).where(User.email == email)
        res = await db.execute(stmt)
        user = res.scalar_one_or_none()

        if user is None:
            user = User(
                google_id=f"dev-mock-google-id-{uuid4().hex[:12]}",
                email=email,
                display_name=f"Đạo Hữu {role.capitalize()}",
                role=role,
            )
            db.add(user)
            await db.flush()
            await db.commit()
            print("✅ Đã tạo mới tài khoản Dev User:")
            print(f"   - Email: {email}")
            print(f"   - Role: {role}")

            # Lấy lại bản ghi để đảm bảo có ID chính xác
            stmt = select(User).where(User.email == email)
            res = await db.execute(stmt)
            user = res.scalar_one_or_none()
        else:
            if user.role != role:
                user.role = role
                db.add(user)
                await db.commit()
                print(f"🔄 Đã cập nhật quyền thành: {role} cho user {email}")
            else:
                print("ℹ️ Tìm thấy tài khoản Dev User có sẵn:")
                print(f"   - Email: {email}")
                print(f"   - Role: {role}")

        assert user is not None, "Không thể khởi tạo Dev User"
        return user


def main():
    parser = argparse.ArgumentParser(
        description="Sinh JWT Access & Refresh Token cho Mộc Đạo Tu Tiên."
    )
    parser.add_argument(
        "--email",
        type=str,
        default="dev-admin@moctu.com",
        help="Email của tài khoản cần sinh token.",
    )
    parser.add_argument(
        "--role",
        type=str,
        default="admin",
        choices=["user", "admin"],
        help="Quyền của tài khoản.",
    )

    args = parser.parse_args()

    # Chạy vòng lặp sự kiện bất đồng bộ
    try:
        loop = asyncio.get_event_loop()
    except RuntimeError:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

    user = loop.run_until_complete(get_or_create_dev_user(args.email, args.role))

    access_token = create_access_token(user.id, user.role)
    refresh_token = create_refresh_token(user.id)

    print("\n" + "=" * 80)
    print("🔑 MỘC ĐẠO TU TIÊN — DEVELOPMENT JWT TOKEN GENERATOR")
    print("=" * 80)
    print(f"User ID:      {user.id}")
    print(f"Email:        {user.email}")
    print(f"Quyền (Role): {user.role}")
    print("\nACCESS_TOKEN (Dùng để gửi trong header: Authorization: Bearer <token>):")
    print(f"\033[92m{access_token}\033[0m")
    print("\nREFRESH_TOKEN (Dùng để refresh token):")
    print(f"\033[96m{refresh_token}\033[0m")
    print("=" * 80)
    print("\n💡 Hướng dẫn sử dụng trong Swagger UI (/docs) hoặc Postman:")
    print("1. Copy đoạn ACCESS_TOKEN màu xanh lá cây ở trên.")
    print("2. Trong Swagger UI (/docs), nhấn nút 'Authorize' góc trên bên phải.")
    print("3. Dán trực tiếp token vào ô Value và bấm 'Authorize'.")
    print(
        "4. Trong Postman, chọn tab 'Authorization' -> Type: 'Bearer Token' -> Dán token vào."
    )
    print("=" * 80)


if __name__ == "__main__":
    main()
