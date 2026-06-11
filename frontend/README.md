# 📂 Frontend - Mộc Đạo Tu Tiên (Flora Cultivation Web Suite)

Thư mục này chứa mã nguồn giao diện Web App cho dự án **Mộc Đạo Tu Tiên**. Ứng dụng cung cấp bảng điều khiển (Dashboard) thời gian thực hiển thị trạng thái sinh thái của cây, tích lũy điểm Tu Vi (EXP) và các giao diện quản trị cấu hình hệ thống.

---

## ⚡ Công Nghệ Sử Dụng (Tech Stack)

Các công nghệ chính được cấu hình trong [package.json](package.json):
*   **Framework:** **React 18** (đóng gói bằng **Vite 6** giúp tăng tốc độ build và Hot Module Replacement cực nhanh).
*   **Routing:** **React Router DOM v6** phục vụ phân trang và điều hướng.
*   **State Management:** **Zustand** quản lý trạng thái đăng nhập toàn cục siêu nhẹ.
*   **HTTP Client:** **Axios** kết hợp bộ chặn (Interceptors) để tự động gắn mã JWT và tự động gửi refresh token khi token hết hạn.
*   **Styling & Icons:** Sử dụng CSS thuần tối ưu hiệu năng kết hợp bộ icon **Lucide React**.
*   **Thông báo:** **React Hot Toast** cho các micro-animations thông báo.

---

## 📁 Cấu Trúc Thư Mục Chính

Cấu trúc thư mục mã nguồn tại [src/](src):
*   [components/](src/components): Chứa các component giao diện dùng chung (Navbar, Spinner, các Badge thông số cảm biến).
*   [pages/](src/pages): Các trang chính của hệ thống:
    *   `LoginPage.jsx`: Màn hình đăng nhập tích hợp Google OAuth.
    *   `PlantsListPage.jsx`: Màn hình danh sách cây trồng của User (hiện tại ràng buộc đơn chậu).
    *   `ClaimPage.jsx`: Màn hình ghép nối (Pair) thiết bị mới sử dụng Plant Code và Verify Code.
    *   `DashboardPage.jsx`: Dashboard trực quan thời gian thực, hiển thị chỉ số, mức độ Linh Khí, và thanh tiến trình Tu Vi.
    *   `LeaderboardPage.jsx`: Bảng xếp hạng các cây có Tu Vi (EXP) cao nhất hệ thống.
    *   `AdminPage.jsx`: Trang dành cho Quản trị viên (Xem thống kê, quản lý thiết bị, cấu hình EXP và Cảnh Giới).
*   [services/](src/services): Cấu hình Client API tại [api.js](src/services/api.js).

---

## 🔌 Cơ Chế Kết Nối Thời Gian Thực (SSE)

Ứng dụng sử dụng **Server-Sent Events (SSE)** kết nối tới endpoint `/api/events/{plant_id}` của backend để cập nhật dashboard ngay lập tức khi thiết bị gửi dữ liệu lên MQTT Broker mà không cần người dùng tải lại trang:
*   Sự kiện `sensor_update`: Cập nhật lập tức các chỉ số độ ẩm đất, ánh sáng, nhiệt độ, không khí.
*   Sự kiện `exp_update`: Cập nhật điểm Tu Vi tích lũy và đẩy thông báo popup nếu có sự kiện đột phá Cảnh Giới mới.

---

## 🚀 Hướng Dẫn Khởi Chạy (Build & Run Guide)

### 1. Cài đặt các thư viện bổ trợ:
Đảm bảo bạn đã cài đặt Node.js, sau đó chạy lệnh sau trong thư mục `frontend`:
```bash
npm install
```

### 2. Chạy Server Development:
Vite được cấu hình sẵn cơ chế proxy chuyển tiếp các yêu cầu có prefix `/api` sang Backend FastAPI (mặc định tại port `8000`) thông qua cài đặt trong [vite.config.js](vite.config.js):
```bash
npm run dev
```
Giao diện sẽ chạy tại địa chỉ: `http://localhost:5173`.

### 3. Biên dịch Production:
Để đóng gói ứng dụng tối ưu dung lượng cho môi trường production:
```bash
npm run build
```
Thư mục `dist/` được sinh ra sẽ chứa các file HTML, JS, CSS tĩnh sẵn sàng deploy lên các máy chủ web (Nginx, Firebase, Vercel, v.v.).


