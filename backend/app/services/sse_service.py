"""SSE Service — Quản lý Server-Sent Events cho real-time updates."""

import asyncio
import json
import logging
from collections import defaultdict
from uuid import UUID

logger = logging.getLogger(__name__)


class SSEManager:
    """Quản lý các SSE connections theo plant_id.

    Mỗi plant_id có thể có nhiều subscriber (nhiều tab trình duyệt).
    """

    def __init__(self) -> None:
        self._subscribers: dict[UUID, list[asyncio.Queue[str]]] = defaultdict(list)

    def subscribe(self, plant_id: UUID) -> asyncio.Queue[str]:
        """Đăng ký nhận events cho một plant_id. Trả về queue để lắng nghe."""
        queue: asyncio.Queue[str] = asyncio.Queue()
        self._subscribers[plant_id].append(queue)
        logger.info(
            "SSE subscriber added for plant %s (total: %d)",
            plant_id,
            len(self._subscribers[plant_id]),
        )
        return queue

    def unsubscribe(self, plant_id: UUID, queue: asyncio.Queue[str]) -> None:
        """Hủy đăng ký khi client ngắt kết nối."""
        if plant_id in self._subscribers:
            self._subscribers[plant_id] = [
                q for q in self._subscribers[plant_id] if q is not queue
            ]
            if not self._subscribers[plant_id]:
                del self._subscribers[plant_id]
            logger.info("SSE subscriber removed for plant %s", plant_id)

    async def broadcast(self, plant_id: UUID, event_type: str, data: dict) -> None:
        """Gửi event đến tất cả subscribers của một plant_id."""
        message = json.dumps({"event": event_type, "data": data})
        subscribers = self._subscribers.get(plant_id, [])
        for queue in subscribers:
            await queue.put(message)
        if subscribers:
            logger.debug(
                "SSE broadcast '%s' to %d subscribers for plant %s",
                event_type,
                len(subscribers),
                plant_id,
            )


# Singleton instance
sse_manager = SSEManager()
