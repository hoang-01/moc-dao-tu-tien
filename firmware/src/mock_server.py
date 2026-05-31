"""
mock_server.py — Giả lập Backend API và MQTT Agent cho ESP32 test offline

Tính năng:
1. Chạy HTTP Mock Server (POST /api/devices/{plant_code}/telemetry)
2. Chạy MQTT Mock Agent:
   - Kết nối tới MQTT Broker (mặc định localhost:1883)
   - Subscribe topic: devices/+/telemetry
   - Khi nhận dữ liệu -> Tính Tu Vi / Cảnh Giới giả lập
   - Publish kết quả trả về topic: devices/{plant_code}/response

Chạy: uv run python mock_server.py
"""

import sys
import json
import random
import asyncio
import threading
from datetime import datetime
from http.server import BaseHTTPRequestHandler, HTTPServer
from gmqtt import Client as MQTTClient
from gmqtt.mqtt.constants import MQTTv311

# Database giả lập lưu trong bộ nhớ
plant_db = {}

# Mốc Cảnh Giới giống backend thật
def determine_rank(exp):
    if exp < 100:
        return "Phàm Mộc"
    elif exp < 500:
        return "Luyện Khí"
    elif exp < 1500:
        return "Trúc Cơ"
    elif exp < 4000:
        return "Kim Đan"
    elif exp < 8000:
        return "Nguyên Anh"
    elif exp < 15000:
        return "Hóa Thần"
    elif exp < 30000:
        return "Đại Thừa"
    else:
        return "Độ Kiếp"

# Hàm xử lý logic tính EXP
def process_mock_telemetry(plant_code, sensors_list):
    # Khởi tạo nếu chưa có trong DB
    if plant_code not in plant_db:
        plant_db[plant_code] = {"exp": 0, "rank": "Phàm Mộc"}

    # Lấy độ ẩm đất
    soil = 50
    for s in sensors_list:
        if s.get("key") == "soil_moisture":
            soil = s.get("value", 50)
            break

    # Tính quality
    if soil >= 60:
        quality = "EXCELLENT"
        delta = 10
    elif soil >= 40:
        quality = "GOOD"
        delta = 5
    elif soil >= 20:
        quality = "FAIR"
        delta = 0
    else:
        quality = "POOR"
        delta = -3

    # Cập nhật EXP
    old_exp = plant_db[plant_code]["exp"]
    new_exp = max(0, old_exp + delta)
    new_rank = determine_rank(new_exp)

    plant_db[plant_code]["exp"] = new_exp
    plant_db[plant_code]["rank"] = new_rank

    return {
        "quality": quality,
        "delta": delta,
        "total_exp": new_exp,
        "rank_name": new_rank
    }


# =============================================================
# HTTP Mock Server
# =============================================================
class MockHTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        html = """
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <title>Mock Server - Moc Dao Tu Tien</title>
            <style>
                body { font-family: sans-serif; background: #1a1a2e; color: #eee; text-align: center; padding: 50px; }
                .box { background: #16213e; border-radius: 12px; padding: 30px; max-width: 500px; margin: 0 auto; box-shadow: 0 4px 15px rgba(0,0,0,0.3); }
                h1 { color: #4ecca3; margin-top: 0; }
                p { color: #aaa; }
                code { background: #0f3460; padding: 4px 8px; border-radius: 4px; color: #e94560; font-family: monospace; }
            </style>
        </head>
        <body>
            <div class="box">
                <h1>🌱 Mộc Đạo Mock Server</h1>
                <p style="color: #4ecca3;">● HTTP & MQTT Mock Agent đang chạy</p>
                <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">
                <p>HTTP: <code>POST /api/devices/{plant_code}/telemetry</code></p>
                <p>MQTT Subscribe: <code>devices/+/telemetry</code></p>
                <p>MQTT Response: <code>devices/{plant_code}/response</code></p>
            </div>
        </body>
        </html>
        """
        self.wfile.write(html.encode("utf-8"))

    def do_POST(self):
        if not self.path.startswith("/api/devices/") or not self.path.endswith("/telemetry"):
            self.send_response(404)
            self.end_headers()
            return

        parts = self.path.split("/")
        plant_code = parts[3] if len(parts) >= 5 else "UNKNOWN"

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)

        try:
            data = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_response(400)
            self.end_headers()
            return

        res = process_mock_telemetry(plant_code, data.get("sensors", []))

        print(f"\n[HTTP] Nhận telemetry từ: {plant_code}")
        print(f"       EXP: {res['total_exp']} ({res['delta']:+d}), Rank: {res['rank_name']}")

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps({
            "status": "ok",
            "exp_awarded": res["delta"] > 0,
            "message": f"HTTP OK. Chất lượng: {res['quality']}"
        }).encode())

    def log_message(self, format, *args):
        pass

def run_http_server(port):
    server = HTTPServer(("0.0.0.0", port), MockHTTPHandler)
    server.serve_forever()


# =============================================================
# MQTT Mock Agent (gmqtt)
# =============================================================
async def run_mqtt_agent(broker_host, broker_port):
    client = MQTTClient("mocdao-mock-agent")

    def on_connect(c, flags, rc, properties):
        print(f"📡 [MQTT] Kết nối thành công Broker {broker_host}:{broker_port}!")
        c.subscribe("devices/+/telemetry", qos=1)
        print("📡 [MQTT] Đã subscribe topic: devices/+/telemetry")

    async def on_message(c, topic, payload, qos, properties):
        try:
            parts = topic.split("/")
            if len(parts) != 3 or parts[2] != "telemetry":
                return
            
            plant_code = parts[1]
            data = json.loads(payload.decode("utf-8"))
            sensors = data.get("sensors", [])

            # Xử lý tính điểm
            res = process_mock_telemetry(plant_code, sensors)

            print(f"\n[MQTT] Nhận Telemetry từ: {plant_code}")
            print(f"       EXP: {res['total_exp']} ({res['delta']:+d}), Rank: {res['rank_name']}")

            # Bắn ngược response về cho ESP32
            response_topic = f"devices/{plant_code}/response"
            response_payload = {
                "total_exp": res["total_exp"],
                "rank_name": res["rank_name"],
                "exp_awarded": res["delta"] > 0,
                "delta_exp": res["delta"]
            }

            c.publish(response_topic, json.dumps(response_payload), qos=1)
            print(f"       Bắn Response -> {response_topic}")

        except Exception as e:
            print(f"[MQTT Error] Lỗi xử lý message: {e}")

    client.on_connect = on_connect
    client.on_message = on_message

    try:
        await client.connect(broker_host, broker_port, version=MQTTv311)
        await asyncio.Event().wait() # Run forever
    except Exception as e:
        print(f"❌ [MQTT] Không thể kết nối tới Broker: {e}")


# =============================================================
# MAIN
# =============================================================
if __name__ == "__main__":
    HTTP_PORT = 8000
    MQTT_HOST = "broker.hivemq.com"
    MQTT_PORT = 1883

    print("🔥 ĐANG KHỞI ĐỘNG MỘC ĐẠO MOCK SERVER & AGENT...")

    # Chạy HTTP Server ở Thread riêng
    http_thread = threading.Thread(target=run_http_server, args=(HTTP_PORT,), daemon=True)
    http_thread.start()
    print(f"✅ HTTP Mock Server đang chạy tại: http://localhost:{HTTP_PORT}")

    # Chạy MQTT Agent ở Main Thread (asyncio loop)
    try:
        asyncio.run(run_mqtt_agent(MQTT_HOST, MQTT_PORT))
    except KeyboardInterrupt:
        print("\n🛑 Mock Server đã dừng.")
        sys.exit(0)
