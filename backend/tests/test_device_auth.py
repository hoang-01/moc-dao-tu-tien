import asyncio
import httpx


async def run_tests():
    base_url = "http://localhost:8000/api/devices"
    print("🚀 Starting Device Authentication API Verification Tests...")

    async with httpx.AsyncClient() as client:
        # Test Case 1: Valid authentication for TESTDEV1 with code 123456
        print("\n--- Test Case 1: Valid Authentication ---")
        payload = {"verify_code": "123456"}
        try:
            response = await client.post(f"{base_url}/TESTDEV1/auth", json=payload)
            print(f"Status Code: {response.status_code}")
            print("Response Body:")
            print(response.json())

            assert response.status_code == 200, "Should return 200 OK"
            data = response.json()
            assert "token" in data, "Should return a JWT token"
            assert data["thresholds"] is not None, "Should return plant thresholds"
            assert data["thresholds"]["temperature_min"] == 18.0, (
                "Should match Kim Tiền temp min"
            )
            print("✅ Test Case 1 Passed!")
        except Exception as e:
            print(f"❌ Test Case 1 Failed: {e}")

        # Test Case 2: Invalid verify_code for TESTDEV1
        print("\n--- Test Case 2: Invalid Verify Code ---")
        payload = {"verify_code": "wrong_code"}
        try:
            response = await client.post(f"{base_url}/TESTDEV1/auth", json=payload)
            print(f"Status Code: {response.status_code}")
            print("Response Body:")
            print(response.json())

            assert response.status_code == 401, "Should return 401 Unauthorized"
            print("✅ Test Case 2 Passed!")
        except Exception as e:
            print(f"❌ Test Case 2 Failed: {e}")

        # Test Case 3: Non-existent device
        print("\n--- Test Case 3: Non-existent Device ---")
        payload = {"verify_code": "123456"}
        try:
            response = await client.post(f"{base_url}/INVALID_DEV/auth", json=payload)
            print(f"Status Code: {response.status_code}")
            print("Response Body:")
            print(response.json())

            assert response.status_code == 404, "Should return 404 Not Found"
            print("✅ Test Case 3 Passed!")
        except Exception as e:
            print(f"❌ Test Case 3 Failed: {e}")


if __name__ == "__main__":
    asyncio.run(run_tests())
