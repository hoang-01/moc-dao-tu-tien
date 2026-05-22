"""Schemas Leaderboard — Bảng xếp hạng."""

import uuid

from pydantic import BaseModel


class LeaderboardEntry(BaseModel):
    """Một dòng trên bảng xếp hạng."""

    rank: int
    plant_name: str
    owner_display_name: str
    total_exp: float
    rank_name: str  # Cảnh Giới hiện tại
    plant_id: uuid.UUID


class LeaderboardResponse(BaseModel):
    """Response bảng xếp hạng."""

    entries: list[LeaderboardEntry]
    total_count: int
