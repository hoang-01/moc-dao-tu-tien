# Tài liệu Đặc tả Phần cứng (Hardware Specification)
# Dự án: Mộc Đạo Tu Tiên (Flora Cultivation)

> **Phiên bản:** 1.0  
> **Ngày tạo:** 2026-04-14  

---

## 1. Mục đích tài liệu
Tài liệu này mô tả chi tiết các linh kiện phần cứng được sử dụng trong hệ thống thiết bị IoT của dự án "Mộc Đạo Tu Tiên", bao gồm chức năng nguyên lý, lý do lựa chọn và tổng quan về giao tiếp kết nối.

## 2. Danh sách linh kiện chi tiết (BOM - Bill of Materials)

Dưới đây là danh sách các linh kiện cốt lõi tạo thành thiết bị phần cứng để đo đạc và gửi dữ liệu về hệ thống.

### 2.1. Vi điều khiển trung tâm (MCU)
* **Tên thiết bị:** Mạch phát triển ESP32-WROOM-32 (ESP32 Development Board)
* **Tính năng chính:**
  * Tích hợp sẵn Wi-Fi và Bluetooth, hoàn hảo cho các dự án IoT để kết nối với Backend Server qua HTTPS.
  * Có hiệu năng xử lý mạnh mẽ, đủ số lượng chân I/O để giao tiếp với toàn bộ module cảm biến và màn hình hiển thị.
  * Hỗ trợ giao tiếp I2C, SPI, UART, Analog/Digital đầy đủ.

### 2.2. Cảm biến độ ẩm đất
* **Tên thiết bị:** Cảm biến độ ẩm đất điện dung (Capacitive Soil Moisture Sensor)
* **Tham khảo:** [Link mua hàng / Xem chi tiết](https://icdayroi.com/cam-bien-do-am-dat-dien-dung)
* **Lý do lựa chọn:** Sử dụng nguyên lý đo điện dung sẽ giúp đầu dò không bị ăn mòn kim loại khi ngâm lâu trong đất có nước (như các loại cảm biến điện trở thông thường), kéo dài tuổi thọ cực kì bền bỉ.
* **Giao tiếp:** Analog (ADC).

### 2.3. Cảm biến ánh sáng 
* **Tên thiết bị:** Cảm biến cường độ ánh sáng BH1750 (GY-302)
* **Tham khảo:** [Link mua hàng / Xem chi tiết](https://nshopvn.com/product/cam-bien-anh-sang-bh1750/)
* **Lý do lựa chọn:** Cảm biến BH1750 trả về đơn vị nhận diện ánh sáng Lux chuẩn trực tiếp với độ phân giải cao (lên đến 65535 lux), thư viện hỗ trợ phong phú và dễ dàng giao tiếp qua I2C. Vượt trội hơn so với việc dùng module quang trở (LDR) chỉ cho ra điện áp biểu thị giá trị tương đối.
* **Giao tiếp:** I2C.
* **Đặc tả Logic**: Cảm biến ánh sáng BH1750 bị loại bỏ (bypassed) khỏi tất cả các logic đánh giá chất lượng tổng hợp môi trường và logic đóng băng Tu Vi trên cả Firmware và Backend để tránh nhiễu phần cứng làm gián đoạn tu luyện. Dữ liệu chỉ dùng lưu trữ thô và hiển thị thô. Thiết bị gửi giá trị `-999.0` khi cảm biến này lỗi.

### 2.4. Cảm biến độ ẩm & nhiệt độ môi trường
* **Tên thiết bị:** Cảm biến DHT22 (AM2302)
* **Lý do lựa chọn:** Cung cấp cả chuẩn đo nhiệt độ và độ ẩm không khí trong một module. DHT22 có dải đo rộng và độ chính xác cao hơn hẳn so với DHT11, giúp phân tích và dự đoán môi trường sống của cây đắc lực hơn.
* **Giao tiếp:** Digital (1-wire protocol đặc thù).

### 2.5. Giao diện hiển thị trên thiết bị
* **Tên thiết bị:** Màn hình LCD Oled 0.96 inch 2 màu (Vàng - Xanh dương)
* **Tham khảo:** [Link mua hàng / Xem chi tiết](https://icdayroi.com/man-hinh-lcd-oled-0-96-inch-2-mau-vang-xanh-duong)
* **Giao tiếp:** I2C.
* **Chi tiết hiển thị và các màn hình thiết kế**:
  * **Màn hình Khởi động (Booting)**: Hiển thị chữ "MOC DAO TU TIEN" + thanh tiến trình từ 0% đến 100% cùng hoạt ảnh đâm chồi của mầm cây kéo dài trong 2 giây khi cắm nguồn.
  * **Màn hình Trạng thái WiFi**:
    * *Đang kết nối*: hiển thị SSID và cột sóng WiFi động (frame-by-frame 0-3 vạch).
    * *Kết nối thất bại*: thông báo kết nối lỗi và bộ đếm ngược 3 giây trước lần thử tiếp theo.
    * *AP Config Portal*: hiển thị tên Access Point và IP cấu hình mặc định (`192.168.4.1`) khi chờ người dùng cấu hình WiFi.
  * **Màn hình Chính (Main screen)**: Thiết kế tối giản, loại bỏ hoàn toàn các thông số thô của cảm biến, chỉ bao gồm:
    * *Dòng 1*: Trạng thái môi trường tổng hợp và hệ số tăng giảm EXP: `TT: OPTIMAL (+1.0)` / `TT: GOOD (+0.5)` / `TT: FAIR (+0.0)` / `TT: POOR (-0.3)` / `TT: DANGER (-0.8)` / `TT: SENSOR ERROR` / `TT: OFFLINE`.
    * *Dòng 2*: Bậc cảnh giới hiện tại và EXP tích lũy dạng số thực: `<rank_name> : <total_exp>` (ví dụ: `Luyen Khi : 123.5`).
    * *1/2 Màn hình dưới*: Hoạt ảnh động của mầm cây:
      - Đung đưa lá và bắn các hạt linh khí (particles) bay ngược lên tượng trưng cho hấp thụ linh khí trời đất khi ở trạng thái `OPTIMAL` (3 hạt) hoặc `GOOD` (1 hạt).
      - Đứng yên khi ở trạng thái `FAIR`/`POOR` (lá rủ nhẹ ở POOR).
      - Héo úa và gục đầu đi kèm biểu tượng cảnh báo nhấp nháy ở trạng thái `DANGER`/`ERROR`.
      - Héo úa đi kèm biểu tượng cột sóng WiFi bị gạch chéo nhấp nháy ở trạng thái `OFFLINE`.

---

## 3. Kiến trúc giao tiếp và lắp ráp dự kiến (Expected Topology)

* **Giao thức I2C:** Màn hình OLED và Cảm biến ánh sáng BH1750 sẽ chia sẻ cùng một Data bus (I2C SDA/SCL) giúp tiết kiệm số lượng dây nối với ESP32.
* **Giao thức Analog:** Cảm biến độ ẩm đất điện dung xuất thông tin qua điện áp sẽ được kết nối vào ngõ vào ADC của ESP32.
* **Giao thức Digital:** Cảm biến DHT22 sử dụng một cổng digital output riêng biệt.

## 4. Quản lý nguồn năng lượng (Power Management)
* Việc sử dụng Wifi ESP32, bộ cảm biến cộng thêm màn hình OLED tự phát sáng yêu cầu một lượng điện năng tương đối.
* Hệ thống sẽ cần quyết định chiến lược cấp điện tùy thuộc vào phiên bản thiết bị (Cắm dây nguồn trực tiếp 5V hoặc dùng Pin sạc 18650/LiPo đi kèm mạch sạc tăng áp TP4056). Thiết kế chi tiết này sẽ được bổ sung tiếp ở quá trình làm mạch và vẽ Schematic.
