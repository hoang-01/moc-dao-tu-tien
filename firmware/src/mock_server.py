"""
mock_server.py — Giả lập Backend API cho ESP32 test offline

Endpoint giả lập đúng chuẩn backend thật:
  POST /api/devices/{plant_code}/telemetry
  Header: X-Plant-Code: {plant_code}
  Body: {"sensors": [{"key": "...", "value": ...}]}

Chạy: python mock_server.py
"""

from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import random


class MockHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        # Hỗ trợ GET request để test kết nối từ xa hoặc debug mạng
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        html = """
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <title>Mock Server - Moc Dao Tu Tien</title>
            <style>
                body { font-family: sans-serif; background: #1a1a2e; color: #eee; text-align: center; padding: 50px; }
                .box { background: #16213e; border-radius: 12px; padding: 30px; max-width: 500px; margin: 0 auto; box-shadow: 0 4px 15px rgba(0,0,0,0.3); }
                h1 { color: #4ecca3; margin-top: 0; }
                p { color: #aaa; font-size: 15px; }
                code { background: #0f3460; padding: 4px 8px; border-radius: 4px; color: #e94560; font-family: monospace; font-size: 14px; }
                .status { color: #4ecca3; font-weight: bold; }
            </style>
        </head>
        <body>
            <div class="box">
                <h1>🌱 Mộc Đạo Mock Server</h1>
                <p class="status">● Đang hoạt động bình thường</p>
                <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">
                <p>Mock Server đã sẵn sàng nhận dữ liệu telemetry từ ESP32 qua phương thức <code>POST</code>.</p>
                <p>Địa chỉ Endpoint: <br><code>POST /api/devices/{plant_code}/telemetry</code></p>
            </div>
        </body>
        </html>
        """
        self.wfile.write(html.encode("utf-8"))

    def do_POST(self):
        # Chỉ xử lý đúng path pattern
        if not self.path.startswith("/api/devices/") or not self.path.endswith("/telemetry"):
            self.send_response(404)
            self.end_headers()
            return

        # Trích xuất plant_code từ URL
        parts = self.path.split("/")
        # /api/devices/{plant_code}/telemetry → index 3
        plant_code_url = parts[3] if len(parts) >= 5 else "UNKNOWN"

        # Kiểm tra header X-Plant-Code
        x_plant_code = self.headers.get("X-Plant-Code", "")
        if x_plant_code != plant_code_url:
            print(f"\n❌ 403: X-Plant-Code '{x_plant_code}' ≠ URL '{plant_code_url}'")
            self.send_response(403)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({
                "detail": "X-Plant-Code không khớp"
            }).encode())
            return

        # Đọc body
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)

        try:
            data = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_response(400)
            self.end_headers()
            return

        # In ra màn hình để debug
        print(f"\n{'='*45}")
        print(f"📡 Nhận telemetry từ: {plant_code_url}")
        print(f"📦 Payload:")
        for s in data.get("sensors", []):
            print(f"   {s['key']:<15} = {s['value']}")

        # Kiểm tra sensors hợp lệ
        valid_keys = {"soil_moisture", "light", "temperature", "humidity"}
        sensor_keys = {s["key"] for s in data.get("sensors", [])}
        if not sensor_keys & valid_keys:
            self.send_response(400)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({
                "detail": "Không có dữ liệu cảm biến hợp lệ"
            }).encode())
            return

        # Tính quality giả lập dựa trên soil_moisture
        soil = next((s["value"] for s in data["sensors"] if s["key"] == "soil_moisture"), 50)
        if soil >= 60:
            quality = "EXCELLENT"
        elif soil >= 40:
            quality = "GOOD"
        elif soil >= 20:
            quality = "FAIR"
        else:
            quality = "POOR"

        # Giả lập anti-spam (30% chance bị skip)
        exp_awarded = random.random() > 0.3

        # Response đúng chuẩn backend
        response = {
            "status": "ok",
            "exp_awarded": exp_awarded,
            "message": f"Xử lý thành công. Chất lượng: {quality}"
        }

        print(f"✅ Response: exp_awarded={exp_awarded}, quality={quality}")

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response).encode())

    def log_message(self, format, *args):
        pass  # Tắt log mặc định của HTTPServer (dùng print thay thế)


if __name__ == "__main__":
    PORT = 8000
    server = HTTPServer(("0.0.0.0", PORT), MockHandler)
    print(f"🔥 Mock Server Mộc Đạo Tu Tiên")
    print(f"   Đang chạy tại: http://0.0.0.0:{PORT}")
    print(f"   Endpoint: POST /api/devices/{{plant_code}}/telemetry")
    print(f"   Ctrl+C để dừng\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Mock Server đã dừng")
