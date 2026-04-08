# Business Requirements Document (BRD)
# Dự án: Mộc Đạo Tu Tiên (Flora Cultivation)

> **Phiên bản:** 1.0  
> **Ngày tạo:** 2026-04-09

---

## 1. Mục tiêu kinh doanh / Mục tiêu dự án

Xây dựng một hệ thống IoT kết hợp Gamification, biến việc chăm sóc cây xanh tại nhà thành trải nghiệm "Tu Tiên" — nơi chất lượng chăm sóc cây ngoài đời thực được phản ánh qua điểm Tu Vi (EXP) của cây.

## 2. Đối tượng người dùng

- Người trồng cây tại nhà muốn có thêm động lực chăm sóc cây thông qua yếu tố game hóa.

## 3. Phạm vi dự án

### 3.1. Hệ thống Đo lường Môi trường
- Thu thập dữ liệu từ cảm biến IoT: **Độ ẩm đất** và **Ánh sáng**.
- Phân loại chất lượng môi trường thành 5 mức: *Rất tốt, Tốt, Bình thường, Xấu, Nguy hiểm*.

### 3.2. Hệ thống Tu Vi (Điểm EXP)
- Điểm Tu Vi tích lũy liên tục theo chu kỳ (phút).
- Môi trường tốt → cộng điểm; môi trường xấu/nguy hiểm → trừ điểm.
- So sánh dữ liệu thực tế với ngưỡng lý tưởng theo loại cây.

### 3.3. Dashboard (Giao diện trạng thái)
- Hiển thị chỉ số môi trường hiện tại (Độ ẩm, Ánh sáng).
- Hiển thị đánh giá chất lượng môi trường.
- Hiển thị tổng điểm Tu Vi của cây.

### 3.4. Định danh & Xác thực
- Đăng nhập bằng **tài khoản Google**.
- Liên kết chậu cây với tài khoản qua **Plant Code** (in trên thiết bị).
- Mỗi tài khoản quản lý **1 chậu cây**.

## 4. Ràng buộc & Giả định

| Hạng mục | Nội dung |
|---|---|
| Phần cứng | Thiết bị IoT cắm trực tiếp vào chậu cây thật |
| Loại cây | Cây tuổi thọ dài (Kim Tiền, Lưỡi Hổ…) |
| Phạm vi | 1 tài khoản — 1 chậu cây |

## 5. Tiêu chí chấp nhận (Acceptance Criteria)

- Cảm biến đo được Độ ẩm đất và Ánh sáng, gửi dữ liệu liên tục.
- Hệ thống phân loại đúng chất lượng môi trường theo ngưỡng của loại cây.
- Điểm Tu Vi được cộng/trừ chính xác theo chất lượng môi trường mỗi chu kỳ.
- Dashboard hiển thị đầy đủ 3 thông tin: chỉ số, đánh giá, tổng điểm.
- Người dùng đăng nhập Google và liên kết chậu cây qua Plant Code thành công.
