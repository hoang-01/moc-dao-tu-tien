def getLinhKhiState(soil, temp):
    # Xử lý các mã lỗi phần cứng
    if temp == -999.0:
        return "Loi: Mat ket noi DHT!"
    if temp == -998.0:
        return "Loi: DHT Du lieu rac!"
    if temp == -997.0:
        return "Loi: Nhiet do nhay vot!"
    if soil == -999.0:
        return "Loi: Cam bien dat!"

    # Logic nghiệp vụ
    if temp > 40.0 or soil <= 5.0:
        return "Tau hoa nhap ma!"  # DANGER
    if soil >= 60.0:
        return "Linh khi doi dao"  # EXCELLENT
    if soil >= 40.0:
        return "Dang phat trien"   # GOOD
    if soil >= 20.0:
        return "Tinh lang tu luyen" # FAIR
    return "Linh khi suy kiet"      # POOR


def run_tests():
    print("="*40)
    print(" BẮT ĐẦU KIỂM THỬ LOGIC NGHIỆP VỤ (WHITE-BOX)")
    print("="*40)

    # Tập dữ liệu kiểm thử (Test Cases)
    test_cases = [
        {"id": "TC_FW_01", "soil": 75.0, "temp": 28.0, "expected_logic": "Linh khi doi dao"},
        {"id": "TC_FW_02", "soil": 45.0, "temp": 30.0, "expected_logic": "Dang phat trien"},
        {"id": "TC_FW_03", "soil": 30.0, "temp": 28.0, "expected_logic": "Tinh lang tu luyen"},
        {"id": "TC_FW_04", "soil": 15.0, "temp": 32.0, "expected_logic": "Linh khi suy kiet"},
        {"id": "TC_FW_05", "soil": 2.0,  "temp": 25.0, "expected_logic": "Tau hoa nhap ma!"},
        {"id": "TC_FW_06", "soil": 50.0, "temp": 42.0, "expected_logic": "Tau hoa nhap ma!"},
        {"id": "TC_FW_07", "soil": 50.0, "temp": -998.0, "expected_logic": "Loi: DHT Du lieu rac!"},
        {"id": "TC_FW_08", "soil": -999.0, "temp": 25.0, "expected_logic": "Loi: Cam bien dat!"},
        {"id": "TC_FW_09", "soil": 50.0, "temp": -997.0, "expected_logic": "Loi: Nhiet do nhay vot!"},
    ]

    passed = 0
    for tc in test_cases:
        result = getLinhKhiState(tc["soil"], tc["temp"])
        status = "PASS" if result == tc["expected_logic"] else "FAIL"
        
        if status == "PASS":
            passed += 1
            print(f"[{status}] {tc['id']}: soil={tc['soil']}, temp={tc['temp']} => {result}")
        else:
            print(f"[{status}] {tc['id']}: soil={tc['soil']}, temp={tc['temp']} => Expected: '{tc['expected_logic']}', Got: '{result}'")

    print("="*40)
    print(f" KẾT QUẢ: {passed}/{len(test_cases)} TEST CASES PASSED")
    print("="*40)


if __name__ == "__main__":
    run_tests()
