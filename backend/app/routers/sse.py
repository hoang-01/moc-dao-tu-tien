"""Router SSE — Server-Sent Events cho Dashboard real-time."""

import asyncio
import logging
from uuid import UUID

from fastapi import APIRouter, Request
from sse_starlette.sse import EventSourceResponse

from app.services.sse_service import sse_manager

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/api/events", tags=["SSE"])


@router.get("/{plant_id}")
async def sse_stream(
    plant_id: UUID,
    request: Request,
) -> EventSourceResponse:
    """SSE stream cho Dashboard real-time.

    Client kết nối để nhận:
    - sensor_update: Dữ liệu cảm biến mới
    - exp_update: Thay đổi Tu Vi & đột phá Cảnh Giới
    """

    async def event_generator():
        queue = sse_manager.subscribe(plant_id)
        try:
            while True:
                # Kiểm tra client còn kết nối không
                if await request.is_disconnected():
                    break

                try:
                    # Chờ message từ queue (timeout 30s để gửi heartbeat)
                    message = await asyncio.wait_for(queue.get(), timeout=30.0)
                    yield {"data": message}
                except asyncio.TimeoutError:
                    # Gửi heartbeat để giữ kết nối
                    yield {"event": "heartbeat", "data": "ping"}
        finally:
            sse_manager.unsubscribe(plant_id, queue)

    return EventSourceResponse(event_generator())
