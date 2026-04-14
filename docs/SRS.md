# Software Requirements Specification (SRS)
# Dự án: Mộc Đạo Tu Tiên (Flora Cultivation)

> **Phiên bản:** 1.0  
> **Ngày tạo:** 2026-04-14  
> **BRD tham chiếu:** v3.1  
> **PRD tham chiếu:** v2.1

---

## Mục lục

1. [Giới thiệu](#1-giới-thiệu)
2. [Mô tả tổng thể hệ thống](#2-mô-tả-tổng-thể-hệ-thống)
3. [Yêu cầu chức năng](#3-yêu-cầu-chức-năng)
4. [Yêu cầu phi chức năng](#4-yêu-cầu-phi-chức-năng)
5. [Mô hình dữ liệu](#5-mô-hình-dữ-liệu)
6. [Giao diện hệ thống](#6-giao-diện-hệ-thống)
7. [Ràng buộc & Giả định](#7-ràng-buộc--giả-định)
8. [Ma trận truy vết yêu cầu](#8-ma-trận-truy-vết-yêu-cầu)
9. [Bảng thuật ngữ](#9-bảng-thuật-ngữ)

---

## 1. Giới thiệu

### 1.1. Mục đích tài liệu

Tài liệu SRS đặc tả chi tiết các yêu cầu kỹ thuật cho hệ thống **Mộc Đạo Tu Tiên** — bao gồm yêu cầu chức năng, yêu cầu phi chức năng, mô hình dữ liệu, giao diện hệ thống và các ràng buộc kỹ thuật. SRS là cầu nối giữa PRD (định nghĩa *cái gì cần xây dựng*) và System Design (định nghĩa *xây dựng như thế nào*).

### 1.2. Phạm vi

Hệ thống gồm 3 thành phần chính:

| Ký hiệu | Thành phần | Mô tả |
|---|---|---|
| **HW** | Thiết bị IoT (Firmware) | Vi điều khiển + cảm biến, gắn vào chậu cây, thu thập dữ liệu môi trường và gửi lên Server |
| **BE** | Backend Server | Nhận dữ liệu từ thiết bị, xử lý logic nghiệp vụ (phân loại, tính điểm), cung cấp API |
| **FE** | Ứng dụng Web (Frontend) | Giao diện cho người dùng cuối (Dashboard) và Admin (Admin Dashboard) |

### 1.3. Đối tượng đọc

- Nhà phát triển phần mềm (Backend, Frontend, Firmware)
- Tester / QA
- Giảng viên hướng dẫn

### 1.4. Tài liệu tham chiếu

| Tài liệu | Phiên bản | Ghi chú |
|---|---|---|
| BRD.md | v3.0 | Mục tiêu kinh doanh, phạm vi dự án |
| PRD.md | v2.0 | Yêu cầu tính năng, user stories, acceptance criteria |

---

## 2. Mô tả tổng thể hệ thống

### 2.1. Kiến trúc hệ thống tổng quan

```
┌────────────┐     HTTPS     ┌──────────────────┐    REST API   ┌──────────────┐
│  Thiết bị  │ ◄───────────► │   Backend        │ ◄───────────► │  Web App     │
│  IoT (HW)  │               │   Server (BE)    │               │  (FE)        │
│            │               │                  │               │              │
│ • Cảm biến │               │ • API Gateway    │               │ • Dashboard  │
│ • MCU      │               │ • Business Logic │               │ • Admin Page │
│ • WiFi     │               │ • Database       │               │              │
└────────────┘               └──────────────────┘               └──────────────┘
                                      │
                                      ▼
                             ┌──────────────────┐
                             │    Database      │
                             └──────────────────┘
```

# 2.2. Luồng dữ liệu chính

```
Cảm biến → MCU đọc giá trị → Gửi lên Server (HTTPS)
    → Server lưu trữ & phân loại chất lượng môi trường
    → Server tính điểm Tu Vi & kiểm tra Đột phá (ngay lập tức)
    → Server trả về kết quả Tu Vi/Cảnh Giới mới cho Thiết bị
    → Frontend hiển thị realtime qua API polling / WebSocket
```

### 2.3. Thông số cảm biến

| Cảm biến | Chỉ số đo | Ký hiệu | Đơn vị | Phạm vi đo | Ghi chú |
|---|---|---|---|---|---|
| Cảm biến độ ẩm đất | Soil Moisture | `soil_moisture` | % | 0–100 | Đo bằng phương pháp điện dung |
| Cảm biến ánh sáng | Light Intensity | `light` | lux | 0–65535 | Đo cường độ ánh sáng xung quanh |
| Cảm biến nhiệt độ không khí | Air Temperature | `temperature` | °C | -40–80 | DHT11/DHT22 |
| Cảm biến độ ẩm không khí | Air Humidity | `humidity` | % | 0–100 | DHT11/DHT22 (chung module với nhiệt độ) |

> **Ghi chú:** Kiến trúc hệ thống thiết kế theo dạng key-value (`sensor_type` → `value`), cho phép bổ sung loại cảm biến mới mà không thay đổi logic cốt lõi.

### 2.4. Chu kỳ hoạt động

| Tham số | Giá trị mặc định | Ghi chú |
|---|---|---|
| Chu kỳ đo & gửi dữ liệu | 60 giây | Thiết bị đo và gửi dữ liệu lên server mỗi 60 giây |
| Timeout mất kết nối | 5 phút | Nếu không nhận được dữ liệu từ thiết bị trong 5 phút → đánh dấu offline |

---

## 3. Yêu cầu chức năng

### SR-01: Đăng nhập & Xác thực

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-01 |
| **Thành phần** | FE, BE |
| **Mô tả** | Xác thực người dùng qua Google OAuth 2.0 |

**Chi tiết kỹ thuật:**

| # | Yêu cầu | Mô tả |
|---|---|---|
| SR-01.1 | Phương thức xác thực | Google OAuth 2.0 (Authorization Code Flow) |
| SR-01.2 | Thông tin lưu trữ | `google_id`, `email`, `display_name`, `avatar_url`, `role` (user/admin) |
| SR-01.3 | Quản lý phiên | JWT (JSON Web Token) với `access_token` (thời hạn ngắn) và `refresh_token` (thời hạn dài) |
| SR-01.4 | Ghi nhớ phiên | `refresh_token` lưu trong HTTP-Only Cookie, tự động gia hạn `access_token` khi hết hạn |
| SR-01.5 | Phân quyền | 2 vai trò: `user` (người dùng cuối) và `admin` (quản trị viên). Vai trò được gán thủ công trong database |

**Luồng xử lý:**

```
1. User nhấn "Đăng nhập bằng Google" trên FE
2. FE redirect đến Google OAuth consent screen
3. User đồng ý → Google redirect về FE kèm authorization code
4. FE gửi authorization code đến BE (POST /api/auth/google)
5. BE đổi authorization code lấy Google access_token
6. BE lấy thông tin user từ Google API (email, name, avatar)
7. BE tạo/cập nhật User record trong DB
8. BE trả về JWT (access_token + refresh_token) cho FE
9. FE lưu token, chuyển hướng đến Dashboard
```

**Xử lý lỗi:**

| Tình huống | Phản hồi |
|---|---|
| Authorization code không hợp lệ | HTTP 401 — "Xác thực thất bại, vui lòng thử lại" |
| Google API không khả dụng | HTTP 502 — "Dịch vụ xác thực tạm thời không khả dụng" |
| Token hết hạn & refresh_token hợp lệ | Tự động cấp access_token mới |
| Token hết hạn & refresh_token không hợp lệ | HTTP 401 — Yêu cầu đăng nhập lại |

---

### SR-02: Liên kết chậu cây (Secure Pairing)

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-02 |
| **Thành phần** | FE, BE |
| **Mô tả** | Liên kết chậu cây với tài khoản người dùng qua Plant Code + Verify Code |

**Mô hình bộ mã thiết bị:**

| Mã | Độ dài | Nơi lưu | Mục đích |
|---|---|---|---|
| **Plant Code** | 8 ký tự alphanumeric (viết hoa) | In trên nhãn thiết bị + DB | Định danh duy nhất thiết bị (UID) |
| **Verify Code** | 6 chữ số | In trên nhãn thiết bị + DB (hash) | Xác thực quyền sở hữu khi liên kết |

**Luồng xử lý:**

```
1. User nhập Plant Code (8 ký tự) và Verify Code (6 số) trên FE
2. FE gửi POST /api/plants/pair { plant_code, verify_code }
3. BE kiểm tra:
   a. plant_code tồn tại trong DB?
   b. verify_code khớp với hash trong DB?
   c. Thiết bị chưa được liên kết với tài khoản khác?
   d. User chưa có chậu cây nào?
4. Nếu hợp lệ → Tạo liên kết (Device ↔ User) trong DB
5. BE trả về HTTP 200 → FE chuyển đến form đặt tên & chọn loại cây
6. User nhập tên cây + chọn loại cây → FE gửi PUT /api/plants/{id}
7. BE tạo record Plant với Tu Vi = 0, Cảnh Giới = mức khởi đầu
```

**Xử lý lỗi:**

| Tình huống | HTTP Code | Phản hồi |
|---|---|---|
| Plant Code không tồn tại | 404 | "Mã thiết bị không hợp lệ" |
| Verify Code sai | 401 | "Mã xác thực không đúng" |
| Thiết bị đã được liên kết | 409 | "Thiết bị này đã được liên kết với tài khoản khác" |
| User đã có chậu cây | 409 | "Tài khoản đã liên kết một chậu cây. Mỗi tài khoản chỉ được liên kết 1 chậu" |
| Thiết bị bị vô hiệu hóa | 403 | "Thiết bị đã bị vô hiệu hóa bởi quản trị viên" |

**Chống Brute-force:**

| Biện pháp | Chi tiết |
|---|---|
| Rate Limiting | Tối đa 5 lần thử liên kết thất bại / IP / 15 phút |
| Lockout | Sau 10 lần thất bại liên tiếp cho cùng Plant Code → khóa Plant Code trong 1 giờ |

---

### SR-03: Thu thập dữ liệu cảm biến

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-03 |
| **Thành phần** | HW, BE |
| **Mô tả** | Thiết bị IoT đo và gửi dữ liệu cảm biến lên server theo chu kỳ |

**Xác thực thiết bị (Device Authentication):**

Thiết bị xác thực bằng mã **Plant Code** được gửi kèm trong header của mỗi yêu cầu telemetry. Do yêu cầu đơn giản hóa, hệ thống tạm thời không sử dụng cơ chế trao đổi token phức tạp.

```
Mỗi chu kỳ 60s:
  POST /api/devices/{plant_code}/telemetry
  Header: X-Plant-Code: {plant_code}
  → Server kiểm tra plant_code tồn tại và đang ACTIVE trong DB
```

| Tham số | Giá trị |
|---|---|
| Giao thức | HTTPS POST |
| Endpoint (HTTP) | `POST /api/devices/{plant_code}/telemetry` |
| Xác thực | `plant_code` trong header `X-Plant-Code` |
| Định dạng dữ liệu | JSON |

**Cấu trúc payload:**

```json
{
  "device_id": "ABC12345",
  "timestamp": "2026-04-14T10:30:00Z",
  "sensors": {
    "soil_moisture": 65.2,
    "light": 12000,
    "temperature": 28.5,
    "humidity": 72.0
  }
}
```

**Xử lý phía Server (BE):**

```
1. Nhận payload từ thiết bị
2. Verify `plant_code` từ header `X-Plant-Code` (kiểm tra tồn tại trong DB)
3. Validate dữ liệu cảm biến (trong phạm vi hợp lệ)
4. Lưu vào bảng SensorReadings
5. **Kiểm tra thời gian (Anti-Spam):**
   - Hệ thống áp dụng quy tắc: **1 tài khoản/thiết bị chỉ được tính điểm Tu Vi tối đa một lần mỗi 60 giây.**
   - **Dung sai (Grace Period):** Để xử lý sai số thời gian của thiết bị hoặc độ trễ mạng (jitter), hệ thống cho phép dung sai **5 giây**. 
   - Nếu (now - last_exp_reward_time) < 55s → Bỏ qua bước tính Tu Vi (SR-05).
   - Nếu (now - last_exp_reward_time) ≥ 55s → Tiếp tục bước 6.
6. Phân loại môi trường & tính điểm Tu Vi (SR-04, SR-05)
7. Kiểm tra đột phá Cảnh Giới
8. Cập nhật trạng thái thiết bị: last_seen = now(), status = ONLINE
9. Trả về thông tin Tu Vi/Cảnh Giới mới trong response
```

**Response payload:**

```json
{
  "status": "success",
  "data": {
    "exp": 110,
    "rank_name": "Luyện Khí",
    "is_breakthrough": false,
    "env_level": "EXCELLENT"
  }
}
```

**Validation dữ liệu cảm biến:**

| Chỉ số | Phạm vi hợp lệ | Xử lý ngoài phạm vi |
|---|---|---|
| soil_moisture | 0–100 (%) | Bỏ qua reading, ghi log cảnh báo |
| light | 0–65535 (lux) | Bỏ qua reading, ghi log cảnh báo |
| temperature | -40–80 (°C) | Bỏ qua reading, ghi log cảnh báo |
| humidity | 0–100 (%) | Bỏ qua reading, ghi log cảnh báo |

---

### SR-04: Phân loại chất lượng môi trường

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-04 |
| **Thành phần** | BE |
| **Mô tả** | So sánh dữ liệu thực tế với ngưỡng lý tưởng theo loại cây, phân loại thành 5 mức |

**5 mức phân loại:**

| Mức | Ký hiệu | Màu hiển thị | Ý nghĩa |
|---|---|---|---|
| 1 | `EXCELLENT` | 🟢 Xanh lá | Môi trường lý tưởng |
| 2 | `GOOD` | 🔵 Xanh dương | Môi trường tốt, gần lý tưởng |
| 3 | `FAIR` | 🟡 Vàng | Môi trường chấp nhận được, cần lưu ý |
| 4 | `POOR` | 🟠 Cam | Môi trường xấu, cần điều chỉnh |
| 5 | `DANGER` | 🔴 Đỏ | Nguy hiểm, cây đang bị tổn hại |

**Ngưỡng lý tưởng & Thuật toán phân loại:**

Mỗi loại cây có ngưỡng cho **từng chỉ số cảm biến** (Admin cấu hình). Ngưỡng gồm 4 khoảng lồng nhau: `excellent ⊂ good ⊂ fair ⊂ poor`, mỗi khoảng có `[min, max]`. Giá trị nằm ngoài khoảng `poor` → `DANGER`.

Phân loại đi từ trong ra ngoài: **nằm trong khoảng hẹp nhất thỏa mãn → lấy mức đó.**

**Phân loại tổng hợp:** Mức tổng hợp = mức **xấu nhất** trong tất cả cảm biến. Cây cần tất cả chỉ số đều tốt để phát triển khỏe mạnh.

---

### SR-05: Hệ thống Tu Vi (EXP) & Cảnh Giới

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-05 |
| **Thành phần** | BE |
| **Mô tả** | Tính điểm Tu Vi theo chu kỳ dựa trên phân loại môi trường; kiểm tra và xử lý đột phá Cảnh Giới |

#### 5a. Tính điểm Tu Vi

> [!IMPORTANT]
> **Quy tắc Anti-Spam (Cơ chế chống gian lận):**
> Để đảm bảo tính công bằng và tránh việc spam dữ liệu nhằm thăng cấp nhanh, hệ thống giới hạn mỗi tài khoản chỉ được cộng/trừ Tu Vi **tối đa một lần sau mỗi 60 giây**. 
> - **Lưu ý về dung sai:** Do yêu cầu thực tế về độ trễ mạng và sai số đồng hồ thiết bị, hệ thống áp dụng mức dung sai **5 giây** (tức là chấp nhận tính điểm nếu lần gửi trước đó cách tối thiểu **55 giây**).
> - Mọi dữ liệu telemetry gửi đến quá dày đặc (< 55s) sẽ được lưu trữ nhưng không kích hoạt logic tính EXP.

**Cơ chế:** Tu Vi được tính và cộng dồn **ngay lập tức** khi Server nhận được dữ liệu Telemetry hợp lệ từ thiết bị (tần suất tiêu chuẩn 60s/lần).

**Hệ số cộng/trừ mặc định (Admin cấu hình được):**

| Mức môi trường | Hệ số (điểm/lần đo) | Ghi chú |
|---|---|---|
| `EXCELLENT` | +2 | Tưởng thưởng cho chăm sóc lý tưởng |
| `GOOD` | +1 | Vẫn tích cực |
| `FAIR` | 0 | Không tăng không giảm |
| `POOR` | -1 | Trừ điểm nhẹ |
| `DANGER` | -5 | Trừ nặng, cây đang bị tổn hại |

> **Lưu ý:** Hệ số được điều chỉnh giảm so với thiết kế cũ (chu kỳ 5 phút) để cân bằng tốc độ thăng cấp khi tần suất gửi dữ liệu tăng lên (mỗi 60s).

**Ràng buộc:**

| Ràng buộc | Giá trị |
|---|---|
| Tu Vi tối thiểu | 0 (không âm) |
| Tu Vi tối đa | Không giới hạn |

**Luồng xử lý (khi nhận Telemetry):**

```
1. **Kiểm tra Anti-Spam:** Nếu (now - last_exp_reward_time) < 55s → Kết thúc luồng tính điểm (giữ nguyên Tu Vi hiện tại). Mức 55s bao gồm 5s dung sai cho jitter mạng.
2. Phân loại chất lượng môi trường dựa trên dữ liệu vừa nhận (SR-04)
3. Tra bảng hệ số → delta_exp
4. new_exp = max(0, current_exp + delta_exp)
5. Cập nhật Tu Vi và thời gian cộng EXP cuối vào DB
6. Kiểm tra đột phá Cảnh Giới (SR-05b)
7. Lưu log tính điểm (thời gian, mức môi trường, delta, exp mới)
```

#### 5b. Hệ thống Cảnh Giới (Rank)

**Bảng Cảnh Giới mặc định (Admin cấu hình được):**

| # | Cảnh Giới | Tu Vi tối thiểu | Mô tả |
|---|---|---|---|
| 1 | Phàm Mộc | 0 | Khởi đầu — cây vừa gia nhập hệ thống |
| 2 | Luyện Khí | 100 | Cây đã quen với môi trường sống |
| 3 | Trúc Cơ | 500 | Cây bắt đầu ổn định, xây nền tảng vững chắc |
| 4 | Kim Đan | 1500 | Cây phát triển mạnh mẽ |
| 5 | Nguyên Anh | 4000 | Cây trưởng thành, sức sống dồi dào |
| 6 | Hóa Thần | 8000 | Cây đạt được trạng thái hài hòa hoàn hảo |
| 7 | Đại Thừa | 15000 | Đỉnh cao tu luyện |
| 8 | Độ Kiếp | 30000 | Cảnh giới tối thượng |

**Thuật toán kiểm tra đột phá:**

```
1. Lấy Cảnh Giới hiện tại và Tu Vi hiện tại của Plant
2. Tìm Cảnh Giới tiếp theo trong bảng (nếu có)
3. Nếu Tu Vi ≥ ngưỡng Cảnh Giới tiếp theo:
   a. Cập nhật Cảnh Giới của Plant
   b. Ghi log đột phá
4. Lưu ý: Có thể đột phá nhiều cảnh giới cùng lúc nếu Tu Vi đủ
   (ví dụ: từ Phàm Mộc nhảy thẳng lên Trúc Cơ nếu Tu Vi ≥ 500)
```

**Thanh tiến trình:**

```
progress_percent = (current_exp - current_rank_min_exp) / (next_rank_min_exp - current_rank_min_exp) * 100

Ví dụ: Tu Vi = 300, Cảnh Giới = Luyện Khí (100), tiếp theo = Trúc Cơ (500)
→ progress = (300 - 100) / (500 - 100) * 100 = 50%
```

---

### SR-06: Dashboard (Giao diện trạng thái)

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-06 |
| **Thành phần** | FE, BE |
| **Mô tả** | Giao diện chính hiển thị trạng thái cây cho người dùng cuối |

**API Endpoint:**

```
GET /api/plants/me/dashboard
```

**Response schema:**

```json
{
  "plant": {
    "id": "uuid",
    "name": "Tiểu Bạch",
    "plant_type": { "id": 1, "name": "Kim Tiền" },
    "exp": 320,
    "rank": { "name": "Luyện Khí", "order": 2 },
    "next_rank": { "name": "Trúc Cơ", "min_exp": 500 },
    "progress_percent": 55.0
  },
  "environment": {
    "quality_level": "GOOD",
    "sensors": [
      { "type": "soil_moisture", "value": 62.5, "unit": "%", "level": "EXCELLENT" },
      { "type": "light", "value": 8500, "unit": "lux", "level": "GOOD" },
      { "type": "temperature", "value": 28.5, "unit": "°C", "level": "EXCELLENT" },
      { "type": "humidity", "value": 72.0, "unit": "%", "level": "GOOD" }
    ],
    "last_updated": "2026-04-14T10:30:00Z"
  },
  "device": {
    "status": "ONLINE",
    "last_seen": "2026-04-14T10:30:00Z"
  },
  ]
}
```

**Yêu cầu hiển thị FE:**

| # | Thành phần | Dữ liệu | Hiển thị |
|---|---|---|---|
| 1 | Thông tin cây | Tên, loại cây | Text |
| 2 | Chỉ số cảm biến | Giá trị từng chỉ số + đơn vị | Số kèm icon cảm biến |
| 3 | Đánh giá chất lượng | Mức phân loại tổng hợp | Màu sắc + icon + text mô tả |
| 4 | Tu Vi | Tổng điểm EXP | Số lớn nổi bật |
| 5 | Cảnh Giới | Tên cảnh giới hiện tại | Badge / label |
| 6 | Thanh tiến trình | % tới cảnh giới tiếp theo | Progress bar |

**Cập nhật dữ liệu:**
- FE poll API mỗi 60 giây (hoặc dùng WebSocket nếu hỗ trợ).

---

### SR-07: Lịch sử & Biểu đồ xu hướng

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-07 |
| **Thành phần** | FE, BE |
| **Mô tả** | Biểu đồ xu hướng các chỉ số cảm biến theo thời gian |

**API Endpoint:**

```
GET /api/plants/me/history?sensor_type={type}&period={period}
```

**Tham số:**

| Tham số | Giá trị | Mô tả |
|---|---|---|
| `sensor_type` | `soil_moisture`, `light`, `temperature`, `humidity`, `all` | Loại cảm biến |
| `period` | `24h`, `7d`, `30d` | Khoảng thời gian |

**Response schema:**

```json
{
  "sensor_type": "soil_moisture",
  "period": "24h",
  "thresholds": {
    "excellent_min": 55,
    "excellent_max": 70
  },
  "data_points": [
    { "timestamp": "2026-04-13T10:30:00Z", "value": 62.5 },
    { "timestamp": "2026-04-13T11:00:00Z", "value": 58.3 }
  ]
}
```

**Yêu cầu hiển thị:**

| # | Yêu cầu |
|---|---|
| 1 | Biểu đồ đường (line chart) cho mỗi chỉ số cảm biến |
| 2 | Vùng ngưỡng lý tưởng (excellent zone) được highlight trên biểu đồ (dải màu xanh mờ) |
| 3 | Người dùng có thể chuyển đổi giữa các khoảng thời gian (24h / 7 ngày / 30 ngày) |
| 4 | Tooltip hiển thị giá trị chính xác khi hover vào điểm dữ liệu |

**Aggregation (gom dữ liệu):**

| Khoảng thời gian | Độ phân giải | Ghi chú |
|---|---|---|
| 24 giờ | 5 phút | Hiển thị đầy đủ |
| 7 ngày | 30 phút | Gom trung bình mỗi 30 phút |
| 30 ngày | 2 giờ | Gom trung bình mỗi 2 giờ |

---

---

### SR-08: Bảng xếp hạng (Leaderboard)

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-08 |
| **Thành phần** | FE, BE |
| **Mô tả** | Hiển thị bảng xếp hạng cây theo Tu Vi |

**API Endpoint:**

```
GET /api/leaderboard?limit={n}
```

**Response schema:**

```json
{
  "leaderboard": [
    {
      "rank_position": 1,
      "plant_name": "Tiểu Bạch",
      "plant_type": "Kim Tiền",
      "owner_display_name": "Nguyễn Văn A",
      "exp": 4500,
      "rank_name": "Nguyên Anh",
      "is_current_user": false
    }
  ],
  "current_user_position": 15,
  "total_participants": 42
}
```

**Yêu cầu:**

| # | Yêu cầu |
|---|---|
| 1 | Hiển thị Top N cây có Tu Vi cao nhất (mặc định N = 20) |
| 2 | Sắp xếp giảm dần theo Tu Vi |
| 3 | Mỗi entry hiển thị: vị trí, tên cây, loại cây, tên chủ, Tu Vi, Cảnh Giới |
| 4 | Highlight vị trí của user hiện tại trong bảng |
| 5 | Nếu user không nằm trong Top N → hiển thị thêm 1 dòng riêng ở cuối bảng |

---

### SR-09: Bảng điều khiển Admin (Admin Dashboard)

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-09 |
| **Thành phần** | FE, BE |
| **Mô tả** | Giao diện tổng quan dành cho Admin |

**API Endpoint:**

```
GET /api/admin/dashboard
Authorization: Bearer {admin_token}
```

**Response schema:**

```json
{
  "overview": {
    "total_users": 150,
    "total_devices": 120,
    "devices_online": 98,
    "devices_offline": 22,
    "total_plants_linked": 95
  },
  "charts": {
    "new_users_daily": [
      { "date": "2026-04-13", "count": 5 },
      { "date": "2026-04-14", "count": 3 }
    ],
    "device_status_ratio": { "online": 98, "offline": 22 },
    "rank_distribution": [
      { "rank_name": "Phàm Mộc", "count": 30 },
      { "rank_name": "Luyện Khí", "count": 25 },
      { "rank_name": "Trúc Cơ", "count": 20 }
    ]
  }
}
```

**Thống kê hiển thị:**

| # | Thống kê | Loại biểu đồ |
|---|---|---|
| 1 | Tổng quan (users, devices, plants) | Thẻ số liệu (stat cards) |
| 2 | Người dùng mới theo ngày/tuần | Biểu đồ cột (bar chart) |
| 3 | Tỷ lệ thiết bị online/offline | Biểu đồ tròn (pie chart) |
| 4 | Phân bố Cảnh Giới | Biểu đồ cột ngang (horizontal bar) |

**Phân quyền:** Chỉ user có `role = admin` mới truy cập được. Trả về HTTP 403 cho user thường.

---

### SR-10: Quản lý & Đăng ký Thiết bị (Device Provisioning)

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-10 |
| **Thành phần** | FE (Admin), BE |
| **Mô tả** | Quản lý vòng đời thiết bị IoT |

**API Endpoints:**

| Method | Endpoint | Mô tả |
|---|---|---|
| `POST` | `/api/admin/devices` | Tạo thiết bị mới (auto-gen Plant Code, Verify Code) |
| `GET` | `/api/admin/devices` | Lấy danh sách thiết bị (phân trang) |
| `GET` | `/api/admin/devices/{id}` | Chi tiết một thiết bị |

**Tạo thiết bị mới (POST /api/admin/devices):**

```
1. Server tự sinh:
   - Plant Code: 8 ký tự alphanumeric uppercase (random, unique)
   - Verify Code: 6 chữ số (random)
2. Lưu vào DB: Plant Code (plaintext), Verify Code (bcrypt hash)
3. Response trả về plaintext CẢ 2 mã (chỉ hiển thị 1 lần duy nhất)
4. Admin nạp Plant Code vào firmware
5. Admin in (Plant Code + Verify Code) lên nhãn thiết bị
```

**Danh sách thiết bị (GET /api/admin/devices):**

| Cột hiển thị | Mô tả |
|---|---|
| Plant Code | Mã định danh thiết bị |
| Liên kết | Tên user liên kết (nếu có) hoặc "Chưa liên kết" |
| Lần gửi cuối | Thời gian last_seen |

---

### SR-11: Quản lý Loại cây & Ngưỡng lý tưởng

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-11 |
| **Thành phần** | FE (Admin), BE |
| **Mô tả** | CRUD loại cây và cấu hình ngưỡng lý tưởng |

**API Endpoints:**

| Method | Endpoint | Mô tả |
|---|---|---|
| `GET` | `/api/admin/plant-types` | Danh sách loại cây |
| `POST` | `/api/admin/plant-types` | Thêm loại cây mới |
| `PUT` | `/api/admin/plant-types/{id}` | Cập nhật loại cây (tên + ngưỡng) |
| `DELETE` | `/api/admin/plant-types/{id}` | Xóa loại cây |

**Cấu trúc dữ liệu loại cây:**

```json
{
  "id": 1,
  "name": "Kim Tiền (Zamioculcas)",
  "description": "Cây chịu bóng, ít cần tưới nước",
  "thresholds": [
    {
      "sensor_type": "soil_moisture",
      "excellent_min": 40, "excellent_max": 60,
      "good_min": 30, "good_max": 70,
      "fair_min": 20, "fair_max": 80,
      "poor_min": 10, "poor_max": 90
    },
    {
      "sensor_type": "light",
      "excellent_min": 5000, "excellent_max": 15000,
      "good_min": 2000, "good_max": 25000,
      "fair_min": 500, "fair_max": 35000,
      "poor_min": 100, "poor_max": 50000
    }
  ]
}
```

**Ràng buộc:**

| Ràng buộc | Chi tiết |
|---|---|
| Xóa loại cây đang được sử dụng | Từ chối xóa (HTTP 409) nếu có Plant đang dùng loại cây này |
| Cập nhật ngưỡng | Có hiệu lực ngay lập tức cho lần nhận Telemetry tiếp theo |
| Validation ngưỡng | `excellent_min ≤ excellent_max`, các khoảng phải lồng nhau hợp lệ: excellent ⊂ good ⊂ fair ⊂ poor |

---

### SR-12: Cấu hình Tu Vi & Cảnh Giới

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-12 |
| **Thành phần** | FE (Admin), BE |
| **Mô tả** | Cấu hình hệ số cộng/trừ Tu Vi và bảng Cảnh Giới |

**API Endpoints:**

| Method | Endpoint | Mô tả |
|---|---|---|
| `GET` | `/api/admin/exp-config` | Lấy cấu hình hiện tại (hệ số + bảng cảnh giới) |
| `PUT` | `/api/admin/exp-config` | Cập nhật cấu hình |

**Cấu trúc cấu hình:**

```json
{
  "exp_multipliers": {
    "EXCELLENT": 10,
    "GOOD": 5,
    "FAIR": 1,
    "POOR": -5,
    "DANGER": -15,
    "OFFLINE": 0
  },
  "ranks": [
    { "order": 1, "name": "Phàm Mộc", "min_exp": 0 },
    { "order": 2, "name": "Luyện Khí", "min_exp": 100 },
    { "order": 3, "name": "Trúc Cơ", "min_exp": 500 },
    { "order": 4, "name": "Kim Đan", "min_exp": 1500 },
    { "order": 5, "name": "Nguyên Anh", "min_exp": 4000 },
    { "order": 6, "name": "Hóa Thần", "min_exp": 8000 },
    { "order": 7, "name": "Đại Thừa", "min_exp": 15000 },
    { "order": 8, "name": "Độ Kiếp", "min_exp": 30000 }
  ]
}
```

**Ràng buộc:**

| Ràng buộc | Chi tiết |
|---|---|
| Hiệu lực | Thay đổi có hiệu lực ngay từ lần nhận Telemetry tiếp theo |
| Ranks | `min_exp` phải tăng dần theo `order`. Rank đầu tiên phải có `min_exp = 0` |
| Hệ số | `EXCELLENT` > `GOOD` > `FAIR` ≥ 0 ≥ `POOR` > `DANGER`. `OFFLINE` = 0 |
| Giữ cảnh giới | Khi thay đổi mốc mà cây hiện tại có Tu Vi vượt mốc mới → cập nhật cảnh giới tương ứng trong lần nhận Telemetry tiếp theo |

---

### SR-13: Chỉnh sửa hồ sơ cây

| Mục | Nội dung |
|---|---|
| **PRD ref** | F-02 (phần chỉnh sửa) |
| **Thành phần** | FE, BE |
| **Mô tả** | Người dùng có thể thay đổi tên và loại cây sau khi liên kết |

**API Endpoint:**

```
PUT /api/plants/me
```

**Request body:**

```json
{
  "name": "Tiểu Bạch v2",
  "plant_type_id": 2
}
```

**Ràng buộc:**

| Ràng buộc | Chi tiết |
|---|---|
| Tên cây | 1–50 ký tự, không chứa ký tự đặc biệt ngoài dấu tiếng Việt |
| Loại cây | Phải là ID tồn tại trong bảng PlantTypes |
| Thay đổi loại cây | Tu Vi và Cảnh Giới **giữ nguyên**; hệ thống bắt đầu đánh giá theo ngưỡng lý tưởng mới từ chu kỳ tiếp theo |

---

## 4. Yêu cầu phi chức năng

### NFR-01: Hiệu năng

| Chỉ số | Mục tiêu | Phương pháp đo |
|---|---|---|
| Thời gian phản hồi API | ≤ 500ms (p95) cho các API đọc (Dashboard, Leaderboard) | Load testing |
| Thời gian phản hồi API | ≤ 1000ms (p95) cho các API ghi/admin | Load testing |
| Độ trễ cập nhật Dashboard | ≤ 60 giây từ lúc cảm biến đo đến khi hiển thị trên FE | End-to-end testing |
| Throughput nhận telemetry | Xử lý ≥ 100 thiết bị gửi dữ liệu đồng thời | Stress testing |

### NFR-02: Khả dụng

| Chỉ số | Mục tiêu |
|---|---|
| Uptime hệ thống | ≥ 95% (cho phép downtime bảo trì theo kế hoạch) |
| Xử lý mất kết nối thiết bị | Hệ thống tiếp tục hoạt động bình thường cho các thiết bị còn lại |
| Khôi phục sau sự cố | Hệ thống khôi phục trong ≤ 30 phút sau khi restart |

### NFR-03: Bảo mật

| Chỉ số | Mục tiêu |
|---|---|
| Xác thực người dùng | Google OAuth 2.0, JWT |
| Xác thực thiết bị | Plant Code (API Key) trong header `X-Plant-Code` |
| Phân quyền API | Middleware kiểm tra role (user/admin) cho mỗi route |
| Mã hóa truyền tải | HTTPS/TLS cho tất cả API endpoints |
| Lưu trữ bí mật | Bcrypt hash cho Verify Code |
| Chống Brute-force | Rate limiting trên endpoint pairing và device auth |

### NFR-04: Trải nghiệm người dùng

| Chỉ số | Mục tiêu |
|---|---|
| Responsive Design | Hoạt động trên desktop (≥1024px) và mobile (≥375px) |
| Thời gian tải trang đầu tiên | ≤ 3 giây (First Contentful Paint) |
| Trực quan | Sử dụng màu sắc và icon nhất quán cho 5 mức phân loại |
| Học cách sử dụng | User mới có thể hiểu Dashboard trong < 2 phút mà không cần hướng dẫn |

### NFR-05: Khả năng cấu hình

| Chỉ số | Mục tiêu |
|---|---|
| Thay đổi ngưỡng loại cây | Có hiệu lực ngay lập tức, không cần redeploy |
| Thay đổi hệ số/mốc cảnh giới | Có hiệu lực ngay lập tức khi nhận dữ liệu mới |
| Thêm loại cảm biến mới | Hỗ trợ bằng cách thêm record vào config, không cần thay đổi logic cốt lõi |

### NFR-06: Khả năng bảo trì

| Chỉ số | Mục tiêu |
|---|---|
| Logging | Ghi log cho: nhận dữ liệu cảm biến, tính điểm, đột phá, lỗi xác thực |
| Cấu trúc mã nguồn | Tách biệt rõ ràng: route → controller → service → repository |
| API Documentation | Tất cả API endpoints có mô tả trong SRS, sẵn sàng cho Swagger/OpenAPI |

---

## 5. Mô hình dữ liệu

### 5.1. Entity Relationship Diagram (ERD)

```
┌─────────────┐       1:N       ┌──────────────┐
│   Users     │────────────────►│   Plants     │
│             │                 │              │
│ id (PK)     │                 │ id (PK)      │
│ google_id   │                 │ user_id (FK) │
│ email       │                 │ device_id(FK)│
│ display_name│                 │ name         │
│ avatar_url  │                 │ plant_type_id│
│ role        │                 │ exp          │
│ created_at  │                 │ rank_order   │
│ updated_at  │                 │ created_at   │
└─────────────┘                 └──────────────┘
                                       │
                                       │ N:1
                                       ▼
┌──────────────┐               ┌──────────────────┐
│   Devices    │               │  PlantTypes      │
│              │               │                  │
│ id (PK)      │               │ id (PK)          │
│ plant_code   │               │ name             │
│ verify_hash  │               │ description      │
└──────────────┘                       │ 1:N
                                       ▼
                               ┌──────────────────┐
                               │  Thresholds      │
                               │                  │
                               │ id (PK)          │
                               │ plant_type_id(FK)│
                               │ sensor_type      │
                               │ excellent_min    │
                               │ excellent_max    │
                               │ good_min         │
                               │ good_max         │
                               │ fair_min         │
                               │ fair_max         │
                               │ poor_min         │
                               │ poor_max         │
                               └──────────────────┘

┌──────────────────┐
│ SensorReadings   │
│                  │
│ id (PK)          │
│ device_id (FK)   │
│ sensor_type      │
│ value            │
│ timestamp        │
└──────────────────┘

┌──────────────────┐
│ ExpLogs          │
│                  │
│ id (PK)          │
│ plant_id (FK)    │
│ env_level        │
│ delta_exp        │
│ new_exp          │
│ calculated_at    │
└──────────────────┘


┌──────────────────┐
│ BreakthroughEvents│
│                   │
│ id (PK)           │
│ plant_id (FK)     │
│ from_rank         │
│ to_rank           │
│ exp_at_event      │
│ created_at        │
└───────────────────┘

┌──────────────────┐
│ ExpConfig        │
│ (singleton)      │
│                  │
│ id (PK)          │
│ multipliers_json │
│ updated_at       │
└──────────────────┘

┌──────────────────┐
│ RankConfig       │
│                  │
│ id (PK)          │
│ order            │
│ name             │
│ min_exp          │
│ updated_at       │
└──────────────────┘
```

### 5.2. Mô tả chi tiết các bảng

#### Users

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | UUID | PK | ID nội bộ |
| google_id | VARCHAR(255) | UNIQUE, NOT NULL | Google account ID |
| email | VARCHAR(255) | UNIQUE, NOT NULL | Email từ Google |
| display_name | VARCHAR(255) | NOT NULL | Tên hiển thị |
| avatar_url | TEXT | NULLABLE | URL ảnh đại diện |
| role | ENUM('user','admin') | NOT NULL, DEFAULT 'user' | Vai trò |
| created_at | TIMESTAMP | NOT NULL | Thời gian tạo |
| updated_at | TIMESTAMP | NOT NULL | Thời gian cập nhật |

#### Devices

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | UUID | PK | ID nội bộ |
| plant_code | CHAR(8) | UNIQUE, NOT NULL | Mã định danh in trên thiết bị |
| verify_hash | VARCHAR(255) | NOT NULL | Bcrypt hash của Verify Code |
| last_seen | TIMESTAMP | NULLABLE | Lần cuối nhận dữ liệu |
| created_at | TIMESTAMP | NOT NULL | Thời gian tạo |

#### Plants

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | UUID | PK | ID nội bộ |
| user_id | UUID | FK → Users.id, UNIQUE | Chủ cây (1 user — 1 plant) |
| device_id | UUID | FK → Devices.id, UNIQUE | Thiết bị liên kết |
| name | VARCHAR(50) | NOT NULL | Tên cây do user đặt |
| plant_type_id | INT | FK → PlantTypes.id | Loại cây |
| exp | INT | NOT NULL, DEFAULT 0, CHECK ≥ 0 | Điểm Tu Vi |
| rank_order | INT | NOT NULL, DEFAULT 1 | Cảnh Giới hiện tại (FK logic → RankConfig.order) |
| created_at | TIMESTAMP | NOT NULL | Thời gian tạo |

#### PlantTypes

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | SERIAL | PK | ID tự tăng |
| name | VARCHAR(100) | UNIQUE, NOT NULL | Tên loại cây |
| description | TEXT | NULLABLE | Mô tả loại cây |
| created_at | TIMESTAMP | NOT NULL | Thời gian tạo |
| updated_at | TIMESTAMP | NOT NULL | Thời gian cập nhật |

#### Thresholds

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | SERIAL | PK | ID tự tăng |
| plant_type_id | INT | FK → PlantTypes.id | Loại cây |
| sensor_type | VARCHAR(50) | NOT NULL | Loại cảm biến (vd: `soil_moisture`, `light`) |
| excellent_min | FLOAT | NOT NULL | Ngưỡng EXCELLENT min |
| excellent_max | FLOAT | NOT NULL | Ngưỡng EXCELLENT max |
| good_min | FLOAT | NOT NULL | Ngưỡng GOOD min |
| good_max | FLOAT | NOT NULL | Ngưỡng GOOD max |
| fair_min | FLOAT | NOT NULL | Ngưỡng FAIR min |
| fair_max | FLOAT | NOT NULL | Ngưỡng FAIR max |
| poor_min | FLOAT | NOT NULL | Ngưỡng POOR min |
| poor_max | FLOAT | NOT NULL | Ngưỡng POOR max |

> **UNIQUE constraint**: (plant_type_id, sensor_type)

#### SensorReadings

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | BIGSERIAL | PK | ID tự tăng |
| device_id | UUID | FK → Devices.id, NOT NULL | Thiết bị gửi |
| sensor_type | VARCHAR(50) | NOT NULL | Loại cảm biến |
| value | FLOAT | NOT NULL | Giá trị đo |
| timestamp | TIMESTAMP | NOT NULL | Thời gian đo (từ thiết bị) |

> **INDEX**: (device_id, timestamp) để query lịch sử nhanh.

#### ExpLogs

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | BIGSERIAL | PK | ID tự tăng |
| plant_id | UUID | FK → Plants.id | Cây |
| env_level | VARCHAR(20) | NOT NULL | Mức phân loại trong chu kỳ này |
| delta_exp | INT | NOT NULL | Điểm cộng/trừ |
| new_exp | INT | NOT NULL | Tu Vi sau tính |
| calculated_at | TIMESTAMP | NOT NULL | Thời gian tính |

#### BreakthroughEvents

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | UUID | PK | ID nội bộ |
| plant_id | UUID | FK → Plants.id | Cây |
| from_rank | INT | NOT NULL | Cảnh Giới trước |
| to_rank | INT | NOT NULL | Cảnh Giới sau |
| exp_at_event | INT | NOT NULL | Tu Vi tại thời điểm đột phá |
| created_at | TIMESTAMP | NOT NULL | Thời gian đột phá |

#### ExpConfig (Singleton — chỉ có 1 record)

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | INT | PK, DEFAULT 1, CHECK = 1 | Luôn = 1 |
| multipliers_json | JSONB | NOT NULL | Hệ số cộng/trừ theo mức |
| updated_at | TIMESTAMP | NOT NULL | Lần cập nhật cuối |

#### RankConfig

| Cột | Kiểu | Ràng buộc | Mô tả |
|---|---|---|---|
| id | SERIAL | PK | ID tự tăng |
| order | INT | UNIQUE, NOT NULL | Thứ tự cảnh giới |
| name | VARCHAR(50) | UNIQUE, NOT NULL | Tên cảnh giới |
| min_exp | INT | NOT NULL, CHECK ≥ 0 | Tu Vi tối thiểu |
| updated_at | TIMESTAMP | NOT NULL | Lần cập nhật cuối |

---

## 6. Giao diện hệ thống

### 6.1. Giao diện giữa Thiết bị IoT và Backend (HW → BE)

| Mục | Nội dung |
|---|---|
| Giao thức | HTTPS POST |
| Xác thực | `plant_code` gửi trong header `X-Plant-Code` |
| Payload | JSON (xem SR-03) |
| Tần suất | Mỗi 60 giây |
| Retry | Nếu gửi thất bại, thiết bị retry sau 10 giây, tối đa 3 lần |

### 6.2. Giao diện giữa Frontend và Backend (FE ↔ BE)

| Mục | Nội dung |
|---|---|
| Giao thức | HTTPS RESTful API |
| Xác thực | JWT Bearer Token trong header `Authorization` |
| Content-Type | `application/json` |
| Phân trang | Query params `?page=1&limit=20` |
| Lỗi chuẩn | JSON `{ "error": { "code": "...", "message": "..." } }` |

**Tổng hợp API Endpoints:**

| # | Method | Endpoint | Mô tả | Auth |
|---|---|---|---|---|
| 1 | POST | `/api/auth/google` | Đăng nhập Google OAuth | Public |
| 2 | POST | `/api/auth/refresh` | Refresh access token | Refresh token |
| 3 | POST | `/api/plants/pair` | Liên kết chậu cây | User |
| 4 | GET | `/api/plants/me/dashboard` | Dashboard data | User |
| 5 | GET | `/api/plants/me/history` | Lịch sử cảm biến | User |
| 6 | PUT | `/api/plants/me` | Cập nhật thông tin cây | User |
| 7 | GET | `/api/leaderboard` | Bảng xếp hạng | User |
| 9 | POST | `/api/devices/{id}/telemetry` | Gửi dữ liệu cảm biến | X-Plant-Code |
| 10 | GET | `/api/admin/dashboard` | Admin Dashboard | Admin |
| 11 | GET | `/api/admin/devices` | Danh sách thiết bị | Admin |
| 12 | POST | `/api/admin/devices` | Tạo thiết bị mới | Admin |
| 15 | GET | `/api/admin/plant-types` | Danh sách loại cây | Admin |
| 16 | POST | `/api/admin/plant-types` | Thêm loại cây | Admin |
| 17 | PUT | `/api/admin/plant-types/{id}` | Cập nhật loại cây | Admin |
| 18 | DELETE | `/api/admin/plant-types/{id}` | Xóa loại cây | Admin |
| 19 | GET | `/api/admin/exp-config` | Lấy cấu hình EXP | Admin |
| 20 | PUT | `/api/admin/exp-config` | Cập nhật cấu hình EXP | Admin |

### 6.3. Mã lỗi chuẩn

| HTTP Code | Ý nghĩa | Khi nào |
|---|---|---|
| 200 | OK | Request thành công |
| 201 | Created | Tạo resource mới thành công |
| 400 | Bad Request | Dữ liệu input không hợp lệ |
| 401 | Unauthorized | Token hết hạn / sai / thiếu |
| 403 | Forbidden | Không đủ quyền (vd: user truy cập admin API) |
| 404 | Not Found | Resource không tồn tại |
| 409 | Conflict | Xung đột (vd: thiết bị đã liên kết) |
| 429 | Too Many Requests | Rate limit exceeded |
| 500 | Internal Server Error | Lỗi server không mong đợi |

---

## 7. Ràng buộc & Giả định

### 7.1. Ràng buộc

| # | Ràng buộc | Chi tiết |
|---|---|---|
| C-01 | Phần cứng | Thiết bị IoT dùng vi điều khiển (ESP32 hoặc tương đương) kết nối WiFi |
| C-02 | Kết nối | Thiết bị cần WiFi để gửi dữ liệu lên server |
| C-03 | Phạm vi | 1 tài khoản — 1 chậu cây, 1 thiết bị — 1 chậu cây |
| C-04 | Xác thực | Chỉ hỗ trợ đăng nhập Google OAuth 2.0 |
| C-05 | Nền tảng | Web App (responsive), không có mobile app native |

### 7.2. Giả định

| # | Giả định |
|---|---|
| A-01 | Thiết bị IoT có nguồn điện ổn định (cắm điện, không dùng pin) |
| A-02 | Mạng WiFi tại vị trí đặt cây đủ ổn định để gửi dữ liệu mỗi 60 giây |
| A-03 | Cây trồng là loại cây tuổi thọ dài, không thay đổi ngưỡng lý tưởng theo mùa |
| A-04 | Số lượng thiết bị trong hệ thống ≤ 500 (quy mô dự án trường học) |
| A-05 | Admin được chỉ định thủ công (không có flow đăng ký admin) |

---

## 8. Ma trận truy vết yêu cầu

Ma trận ánh xạ từ BRD → PRD → SRS để đảm bảo không bỏ sót yêu cầu:

| BRD Section | PRD Feature | SRS Requirement | Trạng thái |
|---|---|---|---|
| 3.1 Đo lường môi trường | F-03 | SR-03: Thu thập dữ liệu | ✅ Đã đặc tả |
| 3.2 Tu Vi & Cảnh Giới | F-05 | SR-05: Hệ thống Tu Vi & Cảnh Giới | ✅ Đã đặc tả |
| 3.3 Dashboard | F-06 | SR-06: Dashboard | ✅ Đã đặc tả |
| 3.4 Lịch sử & Biểu đồ | F-07 | SR-07: Lịch sử & Biểu đồ | ✅ Đã đặc tả |
| 3.5 Hồ sơ cây & Liên kết | F-02 | SR-02: Liên kết chậu cây | ✅ Đã đặc tả |
| 3.6 Định danh & Xác thực | F-01 | SR-01: Đăng nhập & Xác thực | ✅ Đã đặc tả |
| 3.7 Bảng xếp hạng | F-08 | SR-08: Bảng xếp hạng | ✅ Đã đặc tả |
| 3.8 Admin Dashboard | F-09 | SR-09: Admin Dashboard | ✅ Đã đặc tả |
| 3.9 Quản lý Thiết bị IoT | F-10 | SR-10: Device Provisioning | ✅ Đã đặc tả |
| 3.10 Quản lý loại cây | F-11 | SR-11: Quản lý loại cây | ✅ Đã đặc tả |
| 3.11 Cấu hình Tu Vi | F-12 | SR-12: Cấu hình Tu Vi & Cảnh Giới | ✅ Đã đặc tả |
| — | F-04 | SR-04: Phân loại chất lượng | ✅ Đã đặc tả |
| — | — | SR-13: Chỉnh sửa hồ sơ cây | ✅ Đã đặc tả |

---

## 9. Bảng thuật ngữ

| Thuật ngữ | Giải thích |
|---|---|
| **Tu Vi (EXP)** | Điểm kinh nghiệm tích lũy, phản ánh chất lượng chăm sóc cây theo thời gian |
| **Cảnh Giới** | Cấp bậc (rank) của cây, được thăng cấp khi Tu Vi đạt đủ mốc |
| **Đột phá** | Sự kiện cây chuyển từ Cảnh Giới hiện tại lên Cảnh Giới tiếp theo |
| **Plant Code** | Mã định danh duy nhất (8 ký tự alphanumeric) in trên thiết bị, dùng làm API Key cho thiết bị |
| **Verify Code** | Mã xác thực (6 chữ số) in trên thiết bị, dùng khi liên kết |
| **Dashboard** | Giao diện chính hiển thị trạng thái cây, chỉ số, Tu Vi, Cảnh Giới |
| **Ngưỡng lý tưởng** | Khoảng giá trị môi trường tốt nhất cho từng loại cây, dùng để phân loại |
| **Chu kỳ** | Tần suất gửi dữ liệu định kỳ từ thiết bị lên server (mặc định: 60 giây) |
| **Telemetry** | Dữ liệu đo từ cảm biến được gửi từ thiết bị lên server |
| **JWT** | JSON Web Token — phương thức xác thực stateless cho API |
| **OAuth 2.0** | Giao thức ủy quyền chuẩn, dùng để đăng nhập bằng Google |
| **MCU** | Microcontroller Unit — vi điều khiển trên thiết bị IoT |
| **Brute-force** | Tấn công thử-sai liên tục để đoán mã/mật khẩu |
| **Rate Limiting** | Giới hạn số lượng request trong khoảng thời gian nhất định |
| **Singleton** | Bảng/entity chỉ có đúng 1 record duy nhất |
