# 📂 Firmware - Mộc Đạo Tu Tiên (ESP32)

Thư mục này chứa mã nguồn thiết bị IoT (Firmware) của dự án **Mộc Đạo Tu Tiên**. Thiết bị được lập trình trên vi điều khiển ESP32, sử dụng kiến trúc lai song song **HTTP (REST API) + MQTT (Broker)** để mang lại trải nghiệm tối ưu nhất cho người dùng.

---

## 🏗 Kiến trúc & Luồng hoạt động (Hybrid Architecture)

Hệ thống tuân thủ mô hình **Thin IoT Client + Smart Backend**:
*   **HTTP POST (Đồng bộ - Telemetry & Gamification)**:
    *   Mỗi **60 giây** (không chặn - `millis()`), ESP32 gửi dữ liệu cảm biến lên endpoint:
        `POST /api/devices/{plant_code}/telemetry`
        Header: `X-Plant-Code: {plant_code}`
    *   Nhận phản hồi đồng bộ tức thì chứa thông tin Tu Vi được cộng (`exp_awarded`) và trạng thái môi trường để hiển thị lập tức lên màn hình OLED với độ trễ bằng 0.
*   **MQTT (Bất đồng bộ - Trạng thái & Điều khiển)**:
    *   Kết nối nền TCP siêu nhẹ duy trì liên tục với MQTT Broker (Cổng `1883`).
    *   **LWT (Last Will and Testament)**: Đăng ký di chúc `"offline"` tại topic `devices/{plant_code}/status`. Khi kết nối thành công, mạch báo `"online"`. Giúp Web Dashboard cập nhật trạng thái kết nối tức thì.
    *   **Điều khiển từ xa**: Subscribe lắng nghe trên topic `devices/{plant_code}/control`. Khi nhận được lệnh `"water"` hoặc `"tuoi_nuoc"`, mạch lập tức bật bơm tưới nước và kích hoạt hiệu ứng OLED mà không cần chờ đợi hay gửi HTTP polling liên tục.

---

## 🧩 Phần cứng & Sơ đồ chân (Pinout)

**Vi điều khiển:** ESP32 WROOM 32 (hoặc tương thích)

| Linh kiện | Giao tiếp | Chân trên linh kiện | Chân trên ESP32 | Ghi chú |
| :--- | :--- | :--- | :--- | :--- |
| **OLED SSD1306 0.96"** | I2C | VCC <br> GND <br> SCL <br> SDA | 3.3V <br> GND <br> **GPIO 22** <br> **GPIO 21** | Hiển thị Tu Vi, Cảnh Giới và trạng thái tưới |
| **Cảm biến Ánh sáng (TSL2561)** | I2C | VCC <br> GND <br> SCL <br> SDA | 3.3V <br> GND <br> **GPIO 22** <br> **GPIO 21** | Dùng chung đường bus I2C với màn hình OLED |
| **Cảm biến Độ ẩm đất** | Analog | VCC <br> GND <br> AOUT (SIG) | 3.3V <br> GND <br> **GPIO 34** | Đọc ADC (0 - 100%). Cực kỳ an toàn khi dùng chung WiFi |
| **Cảm biến Nhiệt/Ẩm (DHT22)** | Digital | VCC (+) <br> GND (-) <br> DATA (OUT) | 3.3V <br> GND <br> **GPIO 4** | Cảm biến môi trường không khí |

> [!NOTE]
> Nên sử dụng cảm biến độ ẩm đất **điện dung (Capacitive)** để tránh bị ăn mòn kim loại khi cắm lâu trong đất ẩm.

---

## 🛠 Cài đặt & Nạp Code (PlatformIO)

### Bước 1: Chuẩn bị Môi trường
1. Cài đặt **Visual Studio Code** và extension **PlatformIO IDE**.
2. Chọn `File` ➔ `Open Folder...` ➔ Chọn thư mục `firmware` (nơi chứa file `platformio.ini`).
3. PlatformIO sẽ tự động tải các thư viện khai báo (bao gồm `ArduinoJson`, `PubSubClient`, và các thư viện cảm biến Adafruit).

### Bước 2: Cấu hình Fallback (`src/secrets.h`)
Mở file `src/secrets.h` và sửa các thông tin mặc định:
*   `WIFI_SSID` & `WIFI_PASS`: WiFi dùng để test.
*   `SERVER_HOST`: IP máy tính của bạn (VD: `http://192.168.1.33:8000`). Mạch sẽ tự động tách IP này để kết nối cả HTTP (cổng 8000) và MQTT Broker (cổng 1883).

### Bước 3: Nạp Code
1. Cắm ESP32 vào máy tính qua cáp USB.
2. Bấm nút **Upload (Mũi tên `→` ở thanh trạng thái dưới cùng)** để biên dịch và nạp code.
3. Bấm nút **Serial Monitor (Biểu tượng phích cắm `🔌`)** với baudrate `115200` để xem log.

---

## 📱 Cấu hình thiết bị không cần sửa code (Smart Web Portal)

Khi mua thiết bị hoặc mang thiết bị sang môi trường WiFi mới, người dùng không cần sửa code nạp lại:

1. **Khởi chạy Web Portal**: Nếu ESP32 không thể kết nối tới WiFi đã cấu hình sau 3 lần thử (hoặc lần đầu khởi động), mạch sẽ tự động bật điểm thu phát WiFi AP tên là **`MocDao-XXXXXX`** (không có mật khẩu) và màn hình OLED hiển thị hướng dẫn.
2. **Truy cập Giao diện cấu hình**: 
   * Dùng điện thoại kết nối vào WiFi **`MocDao-XXXXXX`**.
   * Mở trình duyệt web bất kỳ truy cập vào địa chỉ IP: `192.168.4.1`.
3. **Nhập cấu hình**:
   * **WiFi SSID**: Tên WiFi nhà bạn.
   * **WiFi Password**: Mật khẩu WiFi.
   * **Plant Code**: Mã định danh duy nhất của chậu cây lấy từ Dashboard Admin trên Web App sau khi tạo chậu cây mới.
4. **Lưu & Khởi động**: Nhấn **Lưu & Khởi động lại**. Thiết bị ghi dữ liệu vào bộ nhớ flash vĩnh viễn (NVS), tự khởi động lại và kết nối vào mạng sử dụng thông tin mới ngay lập tức.

---

## 📡 Chạy Thử và Test Offline (Mock Server)

Khi Backend thật chưa sẵn sàng hoặc bạn đang kiểm tra offline, bạn có thể sử dụng Mock Server giả lập có sẵn:

1. Mở terminal trên máy tính của bạn:
   ```powershell
   cd firmware/src
   python mock_server.py
   ```
2. Điền chính xác IP máy tính của bạn vào `SERVER_HOST` trong `secrets.h` rồi nạp code.
3. Theo dõi log trên terminal `mock_server.py`. Bạn sẽ thấy mạch ESP32 gửi dữ liệu đều đặn, đồng thời Mock Server sẽ trả về các đánh giá môi trường giả lập (Excellent, Good, Fair, Poor) và phân bổ EXP ngẫu nhiên để màn hình OLED hiển thị.
4. Để test MQTT bất đồng bộ, sử dụng công cụ như MQTTX publish lệnh `{"command": "water", "duration": 5}` vào topic `devices/{plant_code}/control` để thấy OLED báo tưới nước tức thì!
