# 📂 Firmware - Mộc Đạo Tu Tiên (ESP32)

Thư mục này chứa mã nguồn thiết bị IoT (Firmware) của dự án **Mộc Đạo Tu Tiên**. Thiết bị được lập trình trên vi điều khiển ESP32, sử dụng giao thức **MQTT (Broker)** để mang lại trải nghiệm thời gian thực tối ưu nhất cho người dùng và luồng giám sát cảnh giới Tu Tiên ảo diệu.

---

## 🏗 Kiến trúc & Luồng hoạt động

Hệ thống tuân thủ mô hình **Thin IoT Client + Smart Backend** với luồng vận hành chi tiết như sau:

### 1. Luồng Khởi động (Boot Sequence)
* **Đọc NVS (Bộ nhớ Flash)**: ESP32 trích xuất cấu hình WiFi và các thông tin xác thực (`verify_code`). Mã thiết bị (`plant_code`) được nạp cứng sẵn trong firmware.
* **Kiểm tra trạng thái**:
  * **Lần đầu khởi động (Chưa có cấu hình)**: Đi vào **Chế độ Cấu hình (Config Mode)**.
  * **Đã có cấu hình**: Thử kết nối WiFi (tối đa 3 lần).
    * Kết nối thành công ➔ Chuyển sang **Trạng thái Hoạt động (Telemetry Loop)**.
    * Kết nối thất bại (sai pass, đổi router...) ➔ Tự động bật **Chế độ Cấu hình** để người dùng nhập lại mạng mới.

### 2. Trạng thái Hoạt động (Telemetry Loop)
Chu kỳ lặp diễn ra đều đặn mỗi **60 giây**:
1. **Đọc Cảm Biến**: Trích xuất dữ liệu từ DHT22, TSL2561, Cảm biến ẩm đất. Kết hợp bộ lọc phần mềm để loại bỏ các dữ liệu rác/nhảy vọt.
2. **Gửi Dữ Liệu (Uplink)**: Đóng gói thành chuẩn JSON và Publish lên MQTT Broker qua topic `devices/{plant_code}/telemetry`.
3. **Đánh giá Cục bộ**: Phân tích tức thời "Trạng Thái Linh Khí" (Ví dụ: Tẩu hỏa nhập ma, Linh khí dồi dào, Tĩnh lặng tu luyện...) dựa trên ngưỡng an toàn của cảm biến.
4. **Hiển thị OLED**: Cập nhật toàn bộ lên giao diện màn hình gồm biểu tượng WiFi/MQTT, thông số môi trường thực và tiến độ Tu Vi.

### 3. Cập nhật Trò chơi (Game State - Downlink)
* ESP32 giữ kết nối Subscribe liên tục tại topic `devices/{plant_code}/response`.
* Bất cứ khi nào Backend xử lý xong logic (sau khi nhận telemetry hoặc do user tương tác trên app), Backend sẽ phản hồi lại thông tin về Tu Vi (`total_exp`) và Cảnh Giới (`rank_name`).
* ESP32 nhận bản tin này và lập tức cập nhật lại thanh tiến trình (Progress Bar) trên OLED mà không cần đợi tới chu kỳ 60 giây tiếp theo.

### 4. Quản lý Sự cố (LWT - Last Will)
* Thiết bị sử dụng cơ chế LWT (Last Will and Testament) của MQTT: Đăng ký sẵn trạng thái `"offline"` tại topic `devices/{plant_code}/status` với Broker ngay lúc mới kết nối.
* Nếu bị rút nguồn điện hoặc mất mạng đột ngột, Broker sẽ tự động phát bản tin `"offline"` báo tử lên Backend, giúp ứng dụng lập tức bôi xám thiết bị.

---

## 🧩 Phần cứng & Sơ đồ chân (Pinout)

**Vi điều khiển:** ESP32 WROOM 32 / ESP32-S3 (hoặc tương thích)

| Linh kiện | Giao tiếp | Chân trên linh kiện | Chân trên ESP32 | Ghi chú |
| :--- | :--- | :--- | :--- | :--- |
| **OLED SSD1306 0.96"** | I2C | VCC <br> GND <br> SCL <br> SDA | 3.3V <br> GND <br> **GPIO 22** <br> **GPIO 21** | Hiển thị Tu Vi, Cảnh Giới và trạng thái tưới |
| **Cảm biến Ánh sáng (TSL2561)** | I2C | VCC <br> GND <br> SCL <br> SDA | 3.3V <br> GND <br> **GPIO 22** <br> **GPIO 21** | Dùng chung đường bus I2C với màn hình OLED |
| **Cảm biến Độ ẩm đất** | Analog | VCC <br> GND <br> AOUT (SIG) | 3.3V <br> GND <br> **GPIO 34** | Đọc ADC (0 - 100%). _Nên sử dụng loại cảm biến Điện dung (Capacitive) chống ăn mòn._ |
| **Cảm biến Nhiệt/Ẩm (DHT22)** | Digital | VCC (+) <br> GND (-) <br> DATA (OUT) | 3.3V <br> GND <br> **GPIO 4** | Cảm biến môi trường không khí chuẩn xác cao |

---

## 🛠 Cài đặt & Nạp Code (PlatformIO)

### Bước 1: Chuẩn bị Môi trường
1. Cài đặt **Visual Studio Code** và extension **PlatformIO IDE**.
2. Chọn `File` ➔ `Open Folder...` ➔ Chọn thư mục `firmware` (nơi chứa file `platformio.ini`).
3. PlatformIO sẽ tự động tải các thư viện khai báo (bao gồm `ArduinoJson`, `PubSubClient`, và các thư viện hỗ trợ phần cứng).

### Bước 2: Nạp Code
1. Cắm cáp Micro-USB / Type-C nối ESP32 với máy tính.
2. Bấm nút **Upload (Mũi tên `→` ở thanh trạng thái màn hình)** để biên dịch và nạp code xuống mạch.
3. Bấm **Serial Monitor (Biểu tượng phích cắm `🔌`)** (Baudrate `115200`) để quan sát luồng khởi động.

---

## 📱 Khởi tạo thiết bị (Smart Auto-Provisioning Web Portal)

Khi triển khai thực tế (Production Deploy) sang môi trường mạng mới, hệ thống áp dụng kỹ thuật Smart Provisioning không cần sửa mã nguồn (Zero-Touch Provisioning):

1. **Khởi chạy Web Portal**: Lần đầu cấp nguồn, nếu ESP32 không tìm thấy mạng nội bộ đã cấu hình, mạch tự động bật điểm thu phát WiFi AP tên là **`MocDao-XXXXXX`** (Không yêu cầu mật khẩu kết nối).
2. **Truy cập Giao diện cấu hình**: 
   * Dùng điện thoại thông minh kết nối vào WiFi AP **`MocDao-XXXXXX`**.
   * Mở trình duyệt web truy cập tự động vào địa chỉ Gateway: `192.168.4.1`.
3. **Nhập cấu hình Đồng bộ**:
   * **WiFi SSID / Password**: Mạng nội bộ nơi đặt cấu hình.
   * **Plant Code**: Mã định danh thiết bị duy nhất lấy từ Control Panel Admin (Dashboard Web).
   * **Mã Bảo Mật (Verify Code)**: Khóa xác thực phần cứng chống thiết bị lạ đính kèm vào hệ thống.
4. **Lưu & Khởi động**: Nhấn gửi. Thiết bị nạp thẳng bản ghi vào bộ nhớ Flash (NVS), bảo vệ dữ liệu xuyên suốt các chu kỳ mất điện, tự tắt Portal và hòa mạng chính thức. Trang bị sẵn tính năng Auto-Reconnect quét kết nối khi đứt cáp viễn thông.
