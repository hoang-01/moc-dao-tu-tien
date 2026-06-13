# Tiêu chí Hoàn thành Phần cứng (Hardware Completion Criteria)

Dự án: **Mộc Đạo Tu Tiên (Flora Cultivation)**

---

## Tiêu chí Chức năng Firmware & Giao thức

- [x] **Cấu hình Wi-Fi:** Tự động phát Wi-Fi AP cấu hình (Captive Portal) qua trang Web khi chưa kết nối được mạng.
- [x] **Lưu trữ:** Lưu Wi-Fi SSID/Pass, Plant Code và Verify Code vào bộ nhớ không biến đổi (NVS/Flash).
- [x] **Hiệu chuẩn độ ẩm đất:** Chuyển đổi giá trị ADC tương ứng về dải 0% - 100%.
- [x] **Gửi dữ liệu:** Đẩy Telemetry định kỳ theo chu kỳ thiết lập qua giao thức MQTT tới topic `devices/{plant_code}/telemetry`.
  - **Định dạng dữ liệu gửi (JSON Payload):**
    ```json
    {
      "sensors": [
        { "key": "soil_moisture", "value": 45.5 },
        { "key": "light", "value": 1500.0 },
        { "key": "temperature", "value": 28.5 },
        { "key": "humidity", "value": 75.0 }
      ]
    }
    ```
- [x] **Giao diện hiển thị OLED (Trực quan & Truyền cảm hứng):**
  - **Thông số cảm biến:** Hiển thị rõ ràng 4 chỉ số (Độ ẩm đất %, Cường độ sáng lux, Nhiệt độ °C, Độ ẩm không khí %).
  - **Trạng thái Tu Tiên (Game State):** Hiển thị Cảnh Giới hiện tại (ví dụ: *Luyện Khí, Trúc Cơ*) và điểm số Tu Vi / Tiến trình đột phá dưới dạng thanh tiến trình (progress bar) hoặc biểu đồ thanh.
  - **Trạng thái Linh Khí (Linh Khí & Sức khỏe):** Trực quan hóa chất lượng môi trường hiện tại bằng thuật ngữ tiên hiệp truyền cảm hứng:
    * *EXCELLENT/GOOD:* "Linh khí dồi dào" (Cây đang phát triển cực tốt).
    * *FAIR:* "Tĩnh lặng tu luyện" (Bình thường).
    * *POOR:* "Linh khí suy kiệt" (Cần chú ý chăm sóc).
    * *DANGER:* "Tẩu hỏa nhập ma" / "Sinh mệnh hấp hối" (Cảnh báo nguy hiểm, cần cứu trợ ngay).
  - **Trạng thái hệ thống:** Hiển thị biểu tượng kết nối mạng (Wi-Fi, MQTT) và hiển thị Plant Code / Verify Code khi thiết bị chưa được liên kết (Unpaired).
- [x] **Chế độ hiển thị liên tục (Always-on Display):** Màn hình OLED hoạt động liên tục (sáng hoài) để hiển thị thông số và trạng thái thời gian thực mà không tự động tắt.
