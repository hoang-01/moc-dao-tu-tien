"""Schemas Auth — Request/Response cho xác thực."""

import uuid
from datetime import datetime

from pydantic import BaseModel, EmailStr


class GoogleLoginRequest(BaseModel):
    """Body nhận Google ID token từ Frontend."""

    id_token: str


class TokenResponse(BaseModel):
    """Response trả JWT tokens."""

    access_token: str
    refresh_token: str
    token_type: str = "bearer"


class RefreshTokenRequest(BaseModel):
    """Body nhận refresh token."""

    refresh_token: str


class UserResponse(BaseModel):
    """Response trả thông tin user hiện tại."""

    id: uuid.UUID
    email: EmailStr
    display_name: str
    avatar_url: str | None
    role: str
    has_plant: bool
    created_at: datetime

    model_config = {"from_attributes": True}
