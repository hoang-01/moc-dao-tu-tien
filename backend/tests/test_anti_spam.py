import asyncio
from unittest.mock import MagicMock
from app.services.exp_service import check_anti_spam, _spam_tracker


async def test_check_anti_spam_rolling_window():
    # Clear tracker to start clean
    _spam_tracker.clear()

    plant = MagicMock()
    plant.id = 999

    # Send 4 requests in rapid succession -> all should succeed (True)
    for i in range(4):
        res = await check_anti_spam(plant)
        print(f"Request {i + 1}: expected True, got {res}")
        assert res is True, f"Request {i + 1} failed"

    # The 5th request in rapid succession -> should be blocked (False)
    res = await check_anti_spam(plant)
    print(f"Request 5: expected False, got {res}")
    assert res is False, "Request 5 should be blocked"

    # Simulate 5.1 seconds passing
    _spam_tracker[plant.id] = [t - 5.1 for t in _spam_tracker[plant.id]]

    # Now, a new request should be allowed
    res = await check_anti_spam(plant)
    print(f"Request after 5.1s: expected True, got {res}")
    assert res is True, "Request after 5.1s failed"

    print("\n[SUCCESS] ALL TESTS PASSED!")


if __name__ == "__main__":
    asyncio.run(test_check_anti_spam_rolling_window())
