# Product Requirements Document (PRD)
# Dự án: Mộc Đạo Tu Tiên (Flora Cultivation)

> **Phiên bản:** 2.1  
> **Ngày tạo:** 2026-04-09  
> **Cập nhật:** 2026-04-14  
> **BRD tham chiếu:** v3.1

---

## 1. Tổng quan sản phẩm

### 1.1. Mô tả sản phẩm

**Mộc Đạo Tu Tiên** là một sản phẩm IoT kết hợp Gamification, biến việc chăm sóc cây xanh tại nhà thành trải nghiệm "Tu Tiên". Hệ thống đo lường các chỉ số môi trường sống của cây, đánh giá chất lượng chăm sóc và phản hồi dưới dạng điểm **Tu Vi (EXP)** cùng hệ thống **Cảnh Giới** (rank) — tạo động lực liên tục cho người dùng.

### 1.2. Thành phần sản phẩm

| Thành phần | Mô tả |
|---|---|
| **Thiết bị IoT** | Cắm vào chậu cây thật, đo các chỉ số môi trường qua cảm biến, gửi dữ liệu lên hệ thống |
| **Ứng dụng Web** | Giao diện cho người dùng cuối: Dashboard, lịch sử, bảng xếp hạng |
| **Trang Admin** | Giao diện cho quản trị viên: giám sát hệ thống, quản lý thiết bị, cấu hình game |

### 1.3. Mục tiêu sản phẩm

| # | Mục tiêu | Đo lường |
|---|---|---|
| O-01 | Tăng động lực chăm sóc cây cho người dùng | Người dùng quay lại kiểm tra Dashboard hàng ngày |
| O-02 | Cung cấp phản hồi rõ ràng về chất lượng môi trường | Người dùng hiểu ngay tình trạng cây mà không cần kiến thức chuyên sâu |
| O-03 | Tạo yếu tố cạnh tranh lành mạnh | Người dùng tham gia bảng xếp hạng, cố gắng nâng Cảnh Giới |
| O-04 | Cho phép vận hành linh hoạt không cần triển khai lại | Admin cấu hình được ngưỡng, loại cây, hệ số điểm qua giao diện |

---

## 2. Đối tượng người dùng & Persona

### 2.1. Persona: Người trồng cây tại nhà

| Thuộc tính | Mô tả |
|---|---|
| **Nhu cầu** | Muốn chăm sóc cây tốt hơn nhưng thiếu động lực hoặc thiếu kiến thức đánh giá môi trường |
| **Hành vi** | Kiểm tra cây không thường xuyên, dễ quên tưới nước hoặc điều chỉnh ánh sáng |
| **Kỳ vọng** | Hệ thống tự động đánh giá và cho phản hồi rõ ràng, dễ hiểu, có yếu tố thú vị |

### 2.2. Persona: Admin / Nhà vận hành

| Thuộc tính | Mô tả |
|---|---|
| **Nhu cầu** | Giám sát toàn bộ hệ thống, phát hiện sớm thiết bị lỗi, cấu hình linh hoạt các tham số game |
| **Hành vi** | Kiểm tra tổng quan hệ thống định kỳ |
| **Kỳ vọng** | Dashboard admin cung cấp thống kê tổng quan, cho phép thao tác quản lý nhanh mà không cần sửa mã nguồn |

---

## 3. Yêu cầu tính năng — Người dùng cuối

### F-01: Đăng nhập & Xác thực

| Mục | Nội dung |
|---|---|
| **Mô tả** | Người dùng đăng nhập bằng tài khoản Google để truy cập hệ thống |
| **Đầu vào** | Tài khoản Google |
| **Đầu ra** | Phiên đăng nhập hợp lệ, truy cập Dashboard |

**User Stories:**
- Là người dùng, tôi muốn đăng nhập bằng Google để không cần tạo tài khoản riêng.
- Là người dùng, tôi muốn hệ thống ghi nhớ phiên đăng nhập để không phải đăng nhập lại mỗi lần mở.

**Tiêu chí chấp nhận:**
- [ ] Đăng nhập thành công bằng Google OAuth.
- [ ] Phiên đăng nhập được lưu, không yêu cầu đăng nhập lại khi mở lại ứng dụng.

---

### F-02: Liên kết chậu cây (Secure Pairing)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Người dùng nhập Plant Code và Verify Code (in trên thiết bị) để liên kết chậu cây với tài khoản. Hệ thống xác thực quyền sở hữu dựa trên mã Verify bí mật |
| **Đầu vào** | Plant Code (Mã định danh), Verify Code (Mã xác thực 6 số) |
| **Đầu ra** | Chậu cây được gắn với tài khoản; Chuyển đến bước đặt tên cây |
| **Cơ chế bảo mật** | Verify Code ngăn chặn việc kẻ tấn công đoán mò Plant Code để liên kết trái phép |
| **Ràng buộc** | Mỗi tài khoản liên kết tối đa **1 chậu cây**; Mỗi bộ mã chỉ được sử dụng thành công 1 lần |

**User Stories:**
- Là người dùng, tôi muốn nhập cả Plant Code và Verify Code để đảm bảo chỉ mình tôi mới có quyền liên kết chậu cây của mình.
- Là người dùng, tôi muốn được thông báo lỗi nếu nhập sai một trong hai mã.
- Là nhà phát triển, tôi muốn ngăn chặn các cuộc tấn công Brute-force vào Plant Code.

**Tiêu chí chấp nhận:**
- [ ] Nhập Plant Code hợp lệ → liên kết thành công, chuyển sang bước đặt tên & chọn loại cây.
- [ ] Plant Code không hợp lệ hoặc đã dùng → hiển thị thông báo lỗi rõ ràng.
- [ ] Người dùng có thể chỉnh sửa tên cây và loại cây bất kỳ lúc nào sau khi liên kết.

---

### F-03: Đo lường môi trường

| Mục | Nội dung |
|---|---|
| **Mô tả** | Thiết bị IoT thu thập dữ liệu các chỉ số môi trường sống của cây (nhiệt độ, độ ẩm không khí, độ ẩm đất, ánh sáng) và gửi lên hệ thống |
| **Cơ chế truyền** | **Delta Sync (Truyền chênh lệch)**: Thiết bị không gửi dữ liệu liên tục định kỳ mà giữ kết nối WiFi và chỉ phát MQTT khi chỉ số thay đổi vượt ngưỡng (nhiệt độ >= 0.5C, độ ẩm >= 3%), hoặc khi thay đổi trạng thái lỗi DHT. Gói tin giữ nhịp kết nối (Heartbeat) được gửi định kỳ mỗi 5 phút. |
| **Cảm biến ánh sáng** | Dữ liệu cảm biến ánh sáng BH1750 được gửi thô lên hệ thống và lưu trữ, nhưng không tham gia đánh giá chất lượng hay logic lỗi để tránh lỗi phần cứng làm gián đoạn tu luyện. |

**User Stories:**
- Là người dùng, tôi muốn xem các chỉ số môi trường hiện tại của cây trên Dashboard.

**Tiêu chí chấp nhận:**
- [ ] Cảm biến đo được các chỉ số môi trường và gửi dữ liệu lên hệ thống thông qua cơ chế Delta Sync.
- [ ] Dữ liệu được cập nhật trên Dashboard thời gian thực với độ trễ tối thiểu.

---

### F-04: Phân loại chất lượng môi trường

| Mục | Nội dung |
|---|---|
| **Mô tả** | Hệ thống so sánh dữ liệu thực tế với ngưỡng lý tưởng theo loại cây và phân loại chất lượng môi trường thành các mức |
| **Phân loại** | 5 mức cục bộ: `EXCELLENT` (đối ứng hiển thị `OPTIMAL`), `GOOD`, `FAIR`, `POOR`, `DANGER`. |
| **Ràng buộc** | Lấy mức xấu nhất trong các cảm biến chính (Nhiệt độ, Độ ẩm không khí, Độ ẩm đất) để làm chất lượng chung. Loại trừ hoàn toàn cảm biến ánh sáng khỏi thuật toán này. |
| **Cơ sở** | Ngưỡng lý tưởng được cấu hình theo loại cây, có thể chỉnh sửa bởi Admin |

**User Stories:**
- Là người dùng, tôi muốn biết môi trường hiện tại của cây đang ở mức nào để biết có cần điều chỉnh không.
- Là người dùng, tôi muốn thấy đánh giá chất lượng dưới dạng trực quan (màu sắc, icon) để nắm bắt nhanh.

**Tiêu chí chấp nhận:**
- [ ] Hệ thống phân loại đúng chất lượng môi trường theo ngưỡng của loại cây đã chọn, không tính cảm biến ánh sáng.
- [ ] Đánh giá được hiển thị bằng màu sắc và icon trực quan trên Dashboard và mạch cục bộ.

---

### F-05: Hệ thống Tu Vi (Điểm EXP) & Cảnh Giới

| Mục | Nội dung |
|---|---|
| **Mô tả** | Điểm Tu Vi được tích lũy liên tục theo chất lượng môi trường mỗi chu kỳ. Khi đạt đủ mốc Tu Vi, cây đột phá Cảnh Giới lên bậc cao hơn |
| **Quy tắc cộng/trừ** | Môi trường tốt (Excellent/Good) → **cộng điểm**; Môi trường trung bình (Fair) → **giữ nguyên**; Môi trường xấu/nguy hiểm (Poor/Danger) → **trừ điểm** |
| **Chu kỳ tích lũy** | Tích lũy mịn cục bộ mỗi 6 giây trên mạch (Excellent: +1.0, Good: +0.5, Fair: +0.0, Poor: -0.3, Danger: -0.8 EXP); Đồng bộ batch định kỳ trên Backend mỗi 1 phút (ở môi trường phát triển). |
| **Chống Spam (Anti-Spam)** | **Rate Limit Rolling Window**: Hệ thống chỉ chặn cộng điểm nếu thiết bị gửi dữ liệu lên >= 5 lần trong 5 giây gần nhất, thay vì chặn cứng 55 giây như cũ. |
| **Đóng băng Tu Vi** | Điểm Tu Vi bị đóng băng (cộng 0 EXP) ngay lập tức nếu thiết bị gặp lỗi cảm biến DHT (`SENSOR_ERROR`) hoặc mất kết nối quá 6 phút (`OFFLINE_PENALTY`). |
| **Đột phá Cảnh Giới** | Khi Tu Vi đạt đủ mốc quy định → cây tự động thăng cấp. Hỗ trợ thuật toán đột phá vượt cấp tự động khi EXP tăng vọt qua nhiều mốc. |

**User Stories:**
- Là người dùng, tôi muốn thấy tổng điểm Tu Vi của cây để biết mình chăm sóc tốt đến mức nào.
- Là người dùng, tôi muốn điểm Tu Vi tăng khi môi trường tốt và giảm khi môi trường xấu để có động lực cải thiện.
- Là người dùng, tôi muốn thấy Cảnh Giới hiện tại của cây và biết cần bao nhiêu Tu Vi nữa để đột phá.
- Là người dùng, tôi muốn được thông báo khi cây đột phá Cảnh Giới để cảm thấy thành tựu.

**Tiêu chí chấp nhận:**
- [ ] Điểm Tu Vi được cộng/trừ chính xác theo chất lượng môi trường mỗi chu kỳ 6s (mạch) hoặc 1 phút (server).
- [ ] Điểm Tu Vi bị đóng băng khi có sự cố cảm biến hoặc thiết bị offline quá 6 phút.
- [ ] Cơ chế Anti-Spam cho phép gửi liên tục cập nhật trạng thái nhưng hạn chế spam điểm vượt mức (5 lần/5s).

---

### F-06: Màn hình hiển thị OLED & Hoạt ảnh Trạng thái

| Mục | Nội dung |
|---|---|
| **Mô tả** | Màn hình OLED trên mạch hiển thị trạng thái tối giản và hoạt ảnh mầm cây đung đưa sống động tương thích với tiến trình Tu Tiên của cây trồng |
| **Màn hình khởi động** | Hiển thị chữ "MOC DAO TU TIEN", tiến trình tải 0% đến 100% và biểu tượng mầm cây chuyển động trong 2 giây. |
| **Màn hình WiFi** | **Đang kết nối**: hiển thị SSID và cột sóng WiFi động.<br>**Kết nối lỗi**: thông báo thất bại và bộ đếm ngược 3 giây trước lần thử tiếp theo.<br>**AP Config Portal**: hiển thị tên Access Point của mạch và IP cấu hình mặc định (192.168.4.1). |
| **Màn hình chính** | Hiển thị tối giản gồm 2 dòng thông tin:<br>1. Trạng thái chất lượng hiện tại: `TT: OPTIMAL/GOOD/FAIR/...` và hệ số EXP biến động (`+1.0`, `+0.5`...).<br>2. Cảnh giới và EXP dạng số thực: `Luyen Khi : 123.5`. |
| **Hoạt ảnh mầm cây** | Một mầm cây động nằm ở 1/2 phía dưới màn hình:<br>- **OPTIMAL/GOOD**: Mầm cây đung đưa vui vẻ, có các hạt linh khí bay lên tượng trưng cho sự tu luyện.<br>- **FAIR/POOR**: Mầm cây đứng yên (POOR thì lá rủ nhẹ).<br>- **DANGER/ERROR**: Mầm cây héo úa, cúi đầu kèm biểu tượng cảnh báo nhấp nháy.<br>- **OFFLINE**: Mầm cây héo úa kèm biểu tượng sóng WiFi bị gạch chéo nhấp nháy. |

**User Stories:**
- Là người dùng, tôi muốn màn hình thiết bị hiển thị sinh động và phản ánh đúng trạng thái tu luyện của cây trồng.
- Là người dùng, tôi muốn dễ dàng cấu hình WiFi cho thiết bị thông qua các thông tin hướng dẫn rõ ràng trên màn hình khi kết nối lỗi.

**Tiêu chí chấp nhận:**
- [ ] Màn hình khởi động, WiFi và màn hình cấu hình hiển thị chính xác các hoạt ảnh và thông số.
- [ ] Mầm cây đung đưa và linh khí bay lên tương ứng đúng với trạng thái môi trường hiện tại.

---

### F-07: Cơ chế Reconnect & Đồng bộ Database

| Mục | Nội dung |
|---|---|
| **Mô tả** | Đảm bảo dữ liệu trên màn hình OLED và Database luôn đồng nhất khi thiết bị mất điện, mất kết nối mạng và khôi phục trở lại |
| **Xác thực lúc khởi động** | Khi bật nguồn, mạch gọi REST HTTP Auth lên server để lấy JWT Token và đồng bộ ngay giá trị `total_exp` và `rank_name` hiện thời từ database. |
| **Tự động Reconnect** | Khi mất WiFi, mạch tự động kích hoạt tiến trình thử kết nối lại 3 lần (mỗi lần chờ tối đa 15s). Nếu thất bại cả 3 lần mới chuyển sang AP Config Portal. |
| **Khôi phục MQTT** | Khi kết nối MQTT được khôi phục, mạch tự động reset bộ đếm gửi tin về `0` để thực hiện gửi telemetry cập nhật chỉ số ngay lập tức. Server nhận tin sẽ tính toán và phản hồi lại giá trị EXP/Rank mới nhất từ DB giúp OLED cập nhật đồng nhất ngay. |

**User Stories:**
- Là người dùng, tôi muốn EXP trên thiết bị không bị reset về 0 khi mất nguồn, và tự động đồng bộ lại chính xác với dữ liệu lưu trên máy chủ khi có mạng lại.

**Tiêu chí chấp nhận:**
- [ ] Thiết bị đồng bộ đúng EXP và Cảnh giới từ DB ngay khi thực hiện HTTP Auth thành công.
- [ ] Thiết bị tự động cập nhật lại thông số mới nhất sau khi khôi phục mạng mà không cần chờ 5 phút.

---

### F-08: Bảng xếp hạng (Leaderboard)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Hiển thị bảng xếp hạng các cây có Tu Vi cao nhất trong hệ thống |
| **Mục đích** | Tạo yếu tố cạnh tranh nhẹ giữa các người dùng, thúc đẩy động lực chăm sóc cây |

**User Stories:**
- Là người dùng, tôi muốn xem bảng xếp hạng để biết cây của mình đứng ở đâu so với người khác.
- Là người dùng, tôi muốn có động lực chăm sóc cây tốt hơn để vượt lên trên bảng xếp hạng.

**Tiêu chí chấp nhận:**
- [ ] Bảng xếp hạng hiển thị đúng thứ tự Tu Vi giữa các người dùng.

---

## 4. Yêu cầu tính năng — Admin / Nhà vận hành

### F-09: Bảng điều khiển Admin (Admin Dashboard)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Giao diện tổng quan toàn hệ thống dành cho Admin |

**Thông tin hiển thị:**

| # | Nội dung | Mô tả |
|---|---|---|
| 1 | Tổng quan | Số lượng người dùng, thiết bị hoạt động, thiết bị offline, tổng cây liên kết |
| 2 | Biểu đồ thống kê | Số người dùng mới theo ngày/tuần, tỷ lệ online/offline, phân bố Cảnh Giới |

**User Stories:**
- Là admin, tôi muốn xem tổng quan hệ thống (số người dùng, thiết bị, cây) trong một màn hình.
- Là admin, tôi muốn xem biểu đồ thống kê để nắm được xu hướng tăng trưởng và tình trạng hệ thống.

**Tiêu chí chấp nhận:**
- [ ] Admin Dashboard hiển thị đúng số liệu tổng quan realtime.
- [ ] Biểu đồ thống kê hiển thị chính xác dữ liệu theo ngày/tuần.

---

### F-10: Quản lý & Đăng ký Thiết bị (Device Provisioning)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Quản lý vòng đời thiết bị: từ khi sản xuất đến khi tới tay người dùng |
| **Chức năng Admin** | 1. **Tạo thiết bị mới:** Hệ thống tự sinh bộ đôi: Plant Code & Verify Code.<br>2. **Quản lý danh sách:** Xem trạng thái, lần cuối gửi dữ liệu. |
| **Quy trình** | Admin tạo bộ mã → Nạp Plant Code vào FW → In (Code+Verify) lên nhãn → Giao cho người dùng |

**User Stories:**
- Là admin, tôi muốn tạo mã thiết bị mới để nạp vào firmware trước khi đóng gói sản phẩm.
- Là admin, tôi muốn xem danh sách thiết bị and trạng thái kết nối để phát hiện thiết bị lỗi.

**Tiêu chí chấp nhận:**
- [ ] Admin xem được danh sách thiết bị với trạng thái kết nối realtime.

---

### F-11: Quản lý Loại cây & Ngưỡng lý tưởng (Plant Profile Management)

| Mục | Nội dung |
|---|---|
| **Mô tả** | CRUD các loại cây được hỗ trợ và cấu hình ngưỡng lý tưởng cho từng loại |
| **Chức năng** | Thêm/Sửa/Xóa loại cây; Cấu hình ngưỡng lý tưởng (nhiệt độ, độ ẩm, ánh sáng…) |
| **Yêu cầu** | Thay đổi có hiệu lực ngay mà không cần triển khai lại hệ thống |

**User Stories:**
- Là admin, tôi muốn thêm loại cây mới vào hệ thống mà không cần sửa mã nguồn.
- Là admin, tôi muốn chỉnh sửa ngưỡng lý tưởng cho loại cây khi cần tinh chỉnh.

**Tiêu chí chấp nhận:**
- [ ] Admin thêm/sửa/xóa loại cây thành công qua giao diện.
- [ ] Thay đổi ngưỡng lý tưởng có hiệu lực ngay lập tức.

---

### F-12: Cấu hình Tu Vi & Cảnh Giới (EXP & Rank Configuration)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Cấu hình các tham số của hệ thống điểm và cảnh giới |
| **Chức năng** | Cấu hình hệ số cộng/trừ Tu Vi theo mức môi trường; Cấu hình mốc Tu Vi cho từng Cảnh Giới |
| **Yêu cầu** | Thay đổi có hiệu lực ngay mà không cần cập nhật mã nguồn |

**User Stories:**
- Là admin, tôi muốn điều chỉnh hệ số cộng/trừ điểm Tu Vi để cân bằng gameplay.
- Là admin, tôi muốn thay đổi mốc Tu Vi cần thiết cho mỗi Cảnh Giới khi cần.

**Tiêu chí chấp nhận:**
- [ ] Admin cấu hình được hệ số Tu Vi và mốc Cảnh Giới qua giao diện.
- [ ] Thay đổi có hiệu lực ngay mà không cần triển khai lại.
� tả** | Giao diện tổng quan toàn hệ thống dành cho Admin |

**Thông tin hiển thị:**

| # | Nội dung | Mô tả |
|---|---|---|
| 1 | Tổng quan | Số lượng người dùng, thiết bị hoạt động, thiết bị offline, tổng cây liên kết |
| 2 | Biểu đồ thống kê | Số người dùng mới theo ngày/tuần, tỷ lệ online/offline, phân bố Cảnh Giới |

**User Stories:**
- Là admin, tôi muốn xem tổng quan hệ thống (số người dùng, thiết bị, cây) trong một màn hình.
- Là admin, tôi muốn xem biểu đồ thống kê để nắm được xu hướng tăng trưởng và tình trạng hệ thống.

**Tiêu chí chấp nhận:**
- [ ] Admin Dashboard hiển thị đúng số liệu tổng quan realtime.
- [ ] Biểu đồ thống kê hiển thị chính xác dữ liệu theo ngày/tuần.

---

### F-11: Quản lý & Đăng ký Thiết bị (Device Provisioning)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Quản lý vòng đời thiết bị: từ khi sản xuất đến khi tới tay người dùng |
| **Chức năng Admin** | 1. **Tạo thiết bị mới:** Hệ thống tự sinh bộ đôi: Plant Code & Verify Code.<br>2. **Quản lý danh sách:** Xem trạng thái, lần cuối gửi dữ liệu. |
| **Quy trình** | Admin tạo bộ mã → Nạp Plant Code vào FW → In (Code+Verify) lên nhãn → Giao cho người dùng |

**User Stories:**
- Là admin, tôi muốn tạo mã thiết bị mới để nạp vào firmware trước khi đóng gói sản phẩm.
- Là admin, tôi muốn xem danh sách thiết bị và trạng thái kết nối để phát hiện thiết bị lỗi.

**Tiêu chí chấp nhận:**
- [ ] Admin xem được danh sách thiết bị với trạng thái kết nối realtime.

---

### F-12: Quản lý Loại cây & Ngưỡng lý tưởng (Plant Profile Management)

| Mục | Nội dung |
|---|---|
| **Mô tả** | CRUD các loại cây được hỗ trợ và cấu hình ngưỡng lý tưởng cho từng loại |
| **Chức năng** | Thêm/Sửa/Xóa loại cây; Cấu hình ngưỡng lý tưởng (nhiệt độ, độ ẩm, ánh sáng…) |
| **Yêu cầu** | Thay đổi có hiệu lực ngay mà không cần triển khai lại hệ thống |

**User Stories:**
- Là admin, tôi muốn thêm loại cây mới vào hệ thống mà không cần sửa mã nguồn.
- Là admin, tôi muốn chỉnh sửa ngưỡng lý tưởng cho loại cây khi cần tinh chỉnh.

**Tiêu chí chấp nhận:**
- [ ] Admin thêm/sửa/xóa loại cây thành công qua giao diện.
- [ ] Thay đổi ngưỡng lý tưởng có hiệu lực ngay lập tức.

---

### F-13: Cấu hình Tu Vi & Cảnh Giới (EXP & Rank Configuration)

| Mục | Nội dung |
|---|---|
| **Mô tả** | Cấu hình các tham số của hệ thống điểm và cảnh giới |
| **Chức năng** | Cấu hình hệ số cộng/trừ Tu Vi theo mức môi trường; Cấu hình mốc Tu Vi cho từng Cảnh Giới |
| **Yêu cầu** | Thay đổi có hiệu lực ngay mà không cần cập nhật mã nguồn |

**User Stories:**
- Là admin, tôi muốn điều chỉnh hệ số cộng/trừ điểm Tu Vi để cân bằng gameplay.
- Là admin, tôi muốn thay đổi mốc Tu Vi cần thiết cho mỗi Cảnh Giới khi cần.

**Tiêu chí chấp nhận:**
- [ ] Admin cấu hình được hệ số Tu Vi và mốc Cảnh Giới qua giao diện.
- [ ] Thay đổi có hiệu lực ngay mà không cần triển khai lại.

---

## 5. User Flow chính

### 5.1. Người dùng cuối

```
┌─────────────┐
│  Mở ứng dụng│
└──────┬──────┘
       ▼
┌──────────────┐    Chưa       ┌─────────────────┐
│ Đã đăng nhập?├──────────────►│ Đăng nhập Google│
└──────┬───────┘               └────────┬────────┘
       │ Rồi                            │
       ▼                                ▼
┌───────────────────┐   Chưa   ┌────────────────────────────┐
│ Đã liên kết cây?  ├─────────►│ Nhập Plant Code            │
└──────┬────────────┘          │ → Đặt tên & Chọn loại cây  │
       │ Rồi                   └────────┬───────────────────┘
       ▼                                ▼
┌────────────────────────────────────────────────┐
│                 DASHBOARD                      │
│                                                │
│  ┌───────────┐ ┌───────────┐ ┌───────────────┐ │
│  │ Chỉ số    │ │ Đánh giá  │ │ Tu Vi & Cảnh  │ │
│  │ Môi trường│ │ Chất lượng│ │ Giới + Tiến   │ │
│  │           │ │  ● Tốt    │ │ trình         │ │
│  └───────────┘ └───────────┘ └───────────────┘ │
│                                                │
│  ┌─────────────────┐  ┌────────────────────┐   │
│  └─────────────────┘  └────────────────────┘   │
└────────────────────────────────────────────────┘
```

### 5.2. Admin

```
┌─────────────┐
│ Đăng nhập   │
│ (Admin)     │
└──────┬──────┘
       ▼
┌──────────────────────────────────────────────┐
│            ADMIN DASHBOARD                   │
│                                              │
│  ┌────────────────┐  ┌─────────────────────┐ │
│  │ Tổng quan      │  │ Biểu đồ thống kê    │ │
│  │ hệ thống       │  │                     │ │
│  └────────────────┘  └─────────────────────┘ │
│                                              │
│  ┌────────────────┐  ┌─────────────────────┐ │
│  │ Quản lý        │  │ Quản lý loại cây    │ │
│  │ Thiết bị       │  │ & Ngưỡng lý tưởng   │ │
│  └────────────────┘  └─────────────────────┘ │
│                                              │
│  ┌─────────────────────────────────────────┐ │
│  │ Cấu hình Tu Vi & Cảnh Giới              │ │
│  └─────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

---

## 6. Yêu cầu phi chức năng

| # | Yêu cầu | Mô tả |
|---|---|---|
| NF-01 | **Cập nhật gần thời gian thực** | Dashboard phản ánh dữ liệu cảm biến với độ trễ chấp nhận được |
| NF-02 | **Dễ sử dụng** | Giao diện đơn giản, trực quan, không cần hướng dẫn phức tạp |
| NF-03 | **Ổn định** | Thiết bị IoT hoạt động liên tục, không cần khởi động lại thường xuyên |
| NF-04 | **Bảo mật** | Xác thực qua Google OAuth; phân quyền rõ ràng giữa người dùng và admin |
| NF-05 | **Khả năng cấu hình** | Các tham số game (ngưỡng, hệ số, mốc cảnh giới) thay đổi được mà không cần triển khai lại |

---

## 7. Ràng buộc & Giả định

| Hạng mục | Nội dung |
|---|---|
| Phần cứng | Thiết bị IoT cắm trực tiếp vào chậu cây thật |
| Loại cây | Cây tuổi thọ dài (Kim Tiền, Lưỡi Hổ…) |
| Phạm vi | 1 tài khoản — 1 chậu cây |
| Kết nối | Thiết bị cần kết nối Internet (WiFi) để gửi dữ liệu |

---

## 8. Ngoài phạm vi (Out of Scope)

Các tính năng sau **không** nằm trong phiên bản này:

- Quản lý nhiều chậu cây trên một tài khoản
- Thông báo đẩy (Push notification) ra ngoài ứng dụng
- Điều khiển tự động (tự tưới nước, tự bật đèn)
- Hỗ trợ đa ngôn ngữ

---

## 9. Bảng thuật ngữ (Glossary)

| Thuật ngữ | Giải thích |
|---|---|
| **Tu Vi (EXP)** | Điểm kinh nghiệm tích lũy, phản ánh chất lượng chăm sóc cây theo thời gian |
| **Cảnh Giới** | Cấp bậc (rank) của cây, được thăng cấp khi Tu Vi đạt đủ mốc |
| **Đột phá** | Sự kiện cây chuyển từ Cảnh Giới hiện tại lên Cảnh Giới tiếp theo |
| **Plant Code** | Mã định danh duy nhất (UID) in trên thiết bị, dùng để nhận diện và xác thực cho thiết bị |
| **Verify Code** | Mã xác thực (PIN) in kèm Plant Code, dùng để chứng minh quyền sở hữu khi liên kết |
| **Dashboard** | Giao diện chính của ứng dụng, hiển thị trạng thái cây và các chỉ số |
| **IoT** | Internet of Things — mạng lưới thiết bị vật lý kết nối Internet |
| **Gamification** | Ứng dụng các yếu tố trò chơi vào hoạt động ngoài game để tăng động lực |
