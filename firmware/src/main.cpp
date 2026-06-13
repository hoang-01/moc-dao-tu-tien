/**
 * main.cpp — Mộc Đạo Tu Tiên | ESP32 Firmware v2.0
 *
 * LUỒNG KHỞI ĐỘNG:
 * ┌─ Boot ─────────────────────────────────────────────────────┐
 * │  1. Đọc NVS (wifi_ssid, wifi_pass, plant_code, server)     │
 * │  2. Nếu chưa có config → vào CONFIG MODE (AP Portal)       │
 * │  3. Nếu có config → thử kết nối WiFi (tối đa 3 lần)       │
 * │     - Thành công → vào TELEMETRY LOOP                      │
 * │     - Thất bại → vào CONFIG MODE để nhập lại               │
 * └────────────────────────────────────────────────────────────┘
 *
 * CONFIG MODE:
 *   ESP32 mở WiFi AP tên "MocDao-XXXXXX"
 *   → User kết nối vào AP đó
 *   → Mở trình duyệt vào 192.168.4.1
 *   → Nhập WiFi SSID, Password, Plant Code, Server URL
 *   → Lưu vào NVS → Restart → Kết nối bình thường
 *
 * TELEMETRY LOOP (mỗi 60s):
 *   Đọc DHT22 → Đọc Soil → Đọc TSL2561
 *   → MQTT Publish: devices/{plant_code}/telemetry
 *   → Hiển thị OLED
 *
 * Thư viện (platformio.ini):
 *   - ArduinoJson, DHT, Adafruit TSL2561, Adafruit SSD1306
 *   - WebServer (built-in ESP32), Preferences (built-in ESP32)
 *   - PubSubClient (MQTT)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
// HTTPClient removed — telemetry now sent via MQTT
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include "secrets.h"

// ── MQTT ──────────────────────────────────────────────────────
#define MQTT_PORT             1883
WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastMqttRetry   = 0;

// ── Thời gian ─────────────────────────────────────────────────
#ifndef TELEMETRY_INTERVAL_MS
#define TELEMETRY_INTERVAL_MS  60000UL  // 60 giây (override trong secrets.h để test nhanh)
#endif
#define WIFI_MAX_RETRY         3          // Số lần thử kết nối WiFi
#define WIFI_RETRY_DELAY_MS    10000      // Mỗi lần thử đợi tối đa 10s
#define CONFIG_TIMEOUT_MS      300000UL  // Portal tự đóng sau 5 phút nếu không có ai vào

// ── OLED ─────────────────────────────────────────────────────
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oledOk = false;
bool bh1750Ok = false; // Trang thai cam bien anh sang

// ── Phần cứng ────────────────────────────────────────────────
DHT dht(PIN_DHT22, DHT22);
BH1750 lightMeter;

// ── NVS ───────────────────────────────────────────────────────
Preferences prefs;

// ── Config Portal Web Server & DNS ────────────────────────────
WebServer configServer(80);
DNSServer dnsServer;

// ── Biến cấu hình (load từ NVS) ───────────────────────────────
String cfg_ssid       = "";
String cfg_pass       = "";
String cfg_plant_code = "";
String cfg_verify_code = "";
String cfg_server     = SERVER_HOST; // Sử dụng SERVER_HOST định nghĩa trong secrets.h
bool   cfg_is_paired  = false;

// ── Biến trạng thái ───────────────────────────────────────────
unsigned long lastSendTime      = 0;
unsigned long lastSampleTime    = 0; // Thoi diem doc cam bien gan nhat
bool          configMode        = false;

// ── Trạng thái màn hình OLED xoay vòng ────────────────────────
unsigned long lastScreenSwitch    = 0;
int           currentScreen       = 0; // 0: Linh Dien (Cam bien), 1: Tien Lo (Gamification)
#define SCREEN_SWITCH_INTERVAL_MS 4000 // Chuyen man hinh moi 4 giay

// ── Cấu trúc dữ liệu ─────────────────────────────────────────
struct SensorData {
    float temperature;
    float humidity;
    float soilMoisture;
    float light;
    bool  isValid;     // Du lieu hop le
    String errorMsg;   // Thong bao loi neu co
};

// ── Nguong thay doi de gui telemetry ──────────────────────────
const float THRESHOLD_TEMP   = 0.5;   // thay doi 0.5 do C
const float THRESHOLD_HUM    = 3.0;   // thay doi 3%
const float THRESHOLD_SOIL   = 2.0;   // thay doi 2%
const float THRESHOLD_LIGHT  = 50.0;  // thay doi 50 lux
const unsigned long MAX_HEARTBEAT_MS = 600000UL; // 10 phut gui 1 lan du khong doi

// ── Gamification ──────────────────────────────────────────────
int           cfg_total_exp = 0;
String        cfg_rank_name = "Pham Moc";
SensorData    lastSensors   = {0.0, 0.0, 0.0, 0.0, false, ""};
SensorData    lastSentData  = {0.0, 0.0, 0.0, 0.0, false, ""};

// =============================================================
// OLED helpers
// =============================================================
void oledClear() {
    if (!oledOk) return;
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
}

void oledShow(const char* line1, const char* line2 = "",
              const char* line3 = "", const char* line4 = "") {
    if (!oledOk) return;
    oledClear();
    oled.setCursor(0, 0);  oled.println(line1);
    oled.setCursor(0, 16); oled.println(line2);
    oled.setCursor(0, 32); oled.println(line3);
    oled.setCursor(0, 48); oled.println(line4);
    oled.display();
}

// =============================================================
// Logic Gamification: Hàm tính mốc EXP kế tiếp
// =============================================================
int getNextRankExp(int current_exp) {
    if (current_exp < 100) return 100;      // Luyện Khí
    if (current_exp < 500) return 500;      // Trúc Cơ
    if (current_exp < 1500) return 1500;    // Kim Đan
    if (current_exp < 4000) return 4000;    // Nguyên Anh
    if (current_exp < 8000) return 8000;    // Hóa Thần
    if (current_exp < 15000) return 15000;  // Đại Thừa
    if (current_exp < 30000) return 30000;  // Độ Kiếp
    return 30000; // Đã đạt đỉnh
}

// =============================================================
// Hàm loại bỏ dấu tiếng Việt để hiển thị sạch trên OLED ASCII
// =============================================================
String removeAccents(String str) {
    struct Replacement {
        const char* utf8Char;
        const char* asciiChar;
    };
    static const Replacement replacements[] = {
        {"á", "a"}, {"à", "a"}, {"ả", "a"}, {"ã", "a"}, {"ạ", "a"},
        {"â", "a"}, {"ấ", "a"}, {"ầ", "a"}, {"ẩ", "a"}, {"ẫ", "a"}, {"ậ", "a"},
        {"ă", "a"}, {"ắ", "a"}, {"ằ", "a"}, {"ẳ", "a"}, {"ẵ", "a"}, {"ặ", "a"},
        {"Á", "A"}, {"À", "A"}, {"Ả", "A"}, {"Ã", "A"}, {"Ạ", "A"},
        {"Â", "A"}, {"Ấ", "A"}, {"Ầ", "A"}, {"Ẩ", "A"}, {"Ẫ", "A"}, {"Ậ", "A"},
        {"Ă", "A"}, {"Ắ", "A"}, {"Ằ", "A"}, {"\u0102", "A"}, {"\u0106", "A"}, {"\u010E", "A"},
        
        {"é", "e"}, {"è", "e"}, {"ẻ", "e"}, {"ẽ", "e"}, {"ẹ", "e"},
        {"ê", "e"}, {"ế", "e"}, {"ề", "e"}, {"ể", "e"}, {"ễ", "e"}, {"ệ", "e"},
        {"É", "E"}, {"È", "E"}, {"Ẻ", "E"}, {"Ẽ", "E"}, {"Ẹ", "E"},
        {"Ê", "E"}, {"Ế", "E"}, {"Ề", "E"}, {"Ể", "E"}, {"Ễ", "E"}, {"Ệ", "E"},
        
        {"í", "i"}, {"ì", "i"}, {"ỉ", "i"}, {"ĩ", "i"}, {"ị", "i"},
        {"Í", "I"}, {"Ì", "I"}, {"Ỉ", "I"}, {"Ĩ", "I"}, {"Ị", "I"},
        
        {"ó", "o"}, {"ò", "o"}, {"ỏ", "o"}, {"õ", "o"}, {"ọ", "o"},
        {"ô", "o"}, {"ố", "o"}, {"ồ", "o"}, {"ổ", "o"}, {"ỗ", "o"}, {"ộ", "o"},
        {"ơ", "o"}, {"ớ", "o"}, {"ờ", "o"}, {"ở", "o"}, {"ỡ", "o"}, {"ợ", "o"},
        {"Ó", "O"}, {"Ò", "O"}, {"Ỏ", "O"}, {"Õ", "O"}, {"Ọ", "O"},
        {"Ô", "O"}, {"Ố", "O"}, {"Ồ", "O"}, {"Ổ", "O"}, {"Ỗ", "O"}, {"Ộ", "O"},
        {"Ơ", "O"}, {"Ớ", "O"}, {"Ờ", "O"}, {"Ở", "O"}, {"Ỡ", "O"}, {"Ợ", "O"},
        
        {"ú", "u"}, {"ù", "u"}, {"ủ", "u"}, {"ũ", "u"}, {"ụ", "u"},
        {"ư", "u"}, {"ứ", "u"}, {"ừ", "u"}, {"ử", "u"}, {"ữ", "u"}, {"ự", "u"},
        {"Ú", "U"}, {"Ù", "U"}, {"Ủ", "U"}, {"Ũ", "U"}, {"Ụ", "U"},
        {"Ư", "U"}, {"Ứ", "U"}, {"Ừ", "U"}, {"Ử", "U"}, {"Ữ", "U"}, {"Ự", "U"},
        
        {"ý", "y"}, {"ỳ", "y"}, {"ỷ", "y"}, {"ỹ", "y"}, {"ỵ", "y"},
        {"Ý", "Y"}, {"Ỳ", "Y"}, {"Ỷ", "Y"}, {"Ỹ", "Y"}, {"Ỵ", "Y"},
        
        {"đ", "d"}, {"Đ", "D"}
    };
    String result = str;
    for (const auto& r : replacements) {
        result.replace(r.utf8Char, r.asciiChar);
    }
    return result;
}

// =============================================================
// Logic Gamification: Lấy Trạng Thái Linh Khí
// =============================================================
String getLinhKhiState(float soil, float temp) {
    // Xử lý các mã lỗi phần cứng từ readSensors()
    if (temp == -999.0) return "Mat DHT22"; 
    if (temp == -998.0) return "DHT Rac";
    if (temp == -997.0) return "DHT Nhay";
    if (soil == -999.0) return "Mat CB Dat";

    if (temp > 40.0 || soil <= 5.0) return "Tau Hoa"; // DANGER
    if (soil >= 60.0) return "Doi Dao"; // EXCELLENT
    if (soil >= 40.0) return "Phat Trien"; // GOOD
    if (soil >= 20.0) return "Tu Luyen"; // FAIR
    return "Suy Kiet"; // POOR
}

// =============================================================
// OLED: Vẽ Header trạng thái và icon kết nối trực quan
// =============================================================
void drawStatusHeader() {
    oled.setTextSize(1);
    oled.setCursor(0, 1);
    
    // 1. Hiển thị thông tin thiết bị gọn gàng bên trái (tối đa 100px)
    if (cfg_is_paired) {
        String shortCode = cfg_plant_code;
        if (shortCode.length() > 8) {
            shortCode = shortCode.substring(shortCode.length() - 8);
        }
        oled.printf("ID: %s", shortCode.c_str());
    } else {
        oled.printf("VCode: %s", cfg_verify_code.c_str());
    }
    
    // 2. Vẽ MQTT Status Dot (tròn đặc nếu connected, tròn rỗng nếu disconnected)
    if (mqttClient.connected()) {
        oled.fillCircle(107, 4, 3, SSD1306_WHITE);
    } else {
        oled.drawCircle(107, 4, 3, SSD1306_WHITE);
    }
    
    // 3. Vẽ WiFi Signal Bars
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    int wx = 115;
    int wy = 1;
    if (wifiOk) {
        oled.fillRect(wx,     wy + 6, 2, 2, SSD1306_WHITE);
        oled.fillRect(wx + 3, wy + 4, 2, 4, SSD1306_WHITE);
        oled.fillRect(wx + 6, wy + 2, 2, 6, SSD1306_WHITE);
    } else {
        // Dấu X báo lỗi WiFi
        oled.drawLine(wx, wy + 2, wx + 6, wy + 8, SSD1306_WHITE);
        oled.drawLine(wx + 6, wy + 2, wx, wy + 8, SSD1306_WHITE);
    }
}

// =============================================================
// OLED: Vẽ màn hình Dashboard (Tu Vi + Cảnh Giới)
// =============================================================
void drawDashboard(unsigned long remainSecs) {
    if (!oledOk) return;
    oledClear();
    
    // Vẽ Header chung và đường kẻ ngăn cách
    drawStatusHeader();
    oled.drawFastHLine(0, 11, 128, SSD1306_WHITE);
    
    char buf[16];
    
    if (currentScreen == 0) {
        // === MÀN HÌNH 1: LINH ĐIỀN ĐO LƯỜNG (GRID LƯỚI 2x2) ===
        oled.drawFastHLine(0, 38, 128, SSD1306_WHITE);
        oled.drawFastVLine(64, 12, 52, SSD1306_WHITE);
        
        // Ô 1: Nhiệt độ (Top-Left)
        oled.setCursor(8, 15); oled.print("Nhiet Do");
        if (lastSensors.temperature <= -997.0) {
            strcpy(buf, "ERR");
        } else {
            snprintf(buf, sizeof(buf), "%.1f C", lastSensors.temperature);
        }
        oled.setCursor((64 - strlen(buf) * 6) / 2, 25);
        oled.print(buf);
        
        // Ô 2: Độ ẩm không khí (Top-Right)
        oled.setCursor(72, 15); oled.print("Am K.Khi");
        if (lastSensors.humidity <= -997.0) {
            strcpy(buf, "ERR");
        } else {
            snprintf(buf, sizeof(buf), "%.0f%%", lastSensors.humidity);
        }
        oled.setCursor(64 + (64 - strlen(buf) * 6) / 2, 25);
        oled.print(buf);
        
        // Ô 3: Độ ẩm đất (Bottom-Left)
        oled.setCursor(14, 41); oled.print("Am Dat");
        if (lastSensors.soilMoisture == -999.0) {
            strcpy(buf, "ERR");
        } else {
            snprintf(buf, sizeof(buf), "%.0f%%", lastSensors.soilMoisture);
        }
        oled.setCursor((64 - strlen(buf) * 6) / 2, 51);
        oled.print(buf);
        
        // Ô 4: Ánh sáng (Bottom-Right)
        oled.setCursor(72, 41); oled.print("Anh Sang");
        if (lastSensors.light <= -997.0) {
            strcpy(buf, "ERR");
        } else {
            snprintf(buf, sizeof(buf), "%.0f lx", lastSensors.light);
        }
        oled.setCursor(64 + (64 - strlen(buf) * 6) / 2, 51);
        oled.print(buf);
        
    } else {
        // === MÀN HÌNH 2: TIÊN LỘ TU LUYỆN (GIÃN CÁCH ĐỀU) ===
        
        // Dòng 1: Cảnh Giới
        oled.setCursor(8, 15);
        String rankNameClean = removeAccents(cfg_rank_name);
        oled.printf("Canh Gioi: %s", rankNameClean.c_str());
        
        // Dòng 2: Trạng Thái Linh Khí
        oled.setCursor(8, 27);
        String lkState = getLinhKhiState(lastSensors.soilMoisture, lastSensors.temperature);
        oled.printf("Linh Khi: %s", removeAccents(lkState).c_str());
        
        // Dòng 3: Điểm Tu Vi
        oled.setCursor(8, 39);
        int nextExp = getNextRankExp(cfg_total_exp);
        oled.printf("Tu Vi: %d/%d", cfg_total_exp, nextExp);
        
        // Dòng 4: Progress Bar Tu Vi ở đáy màn hình
        float progress = (float)cfg_total_exp / nextExp;
        if (progress > 1.0) progress = 1.0;
        int barWidth = (int)(progress * 112); // Chiều rộng tối đa 112px, chừa lề 8px mỗi bên
        
        oled.drawRect(8, 53, 112, 6, SSD1306_WHITE);
        oled.fillRect(8, 53, barWidth, 6, SSD1306_WHITE);
    }
    
    oled.display();
}

// =============================================================
// MQTT: Callback nhận kết quả Tu Vi/Cảnh Giới từ Broker
// =============================================================
// Hàm lưu trạng thái pairing của thiết bị
void savePairingState(bool is_paired) {
    prefs.begin("mocdao", false);
    prefs.putBool("is_paired", is_paired);
    prefs.end();
    cfg_is_paired = is_paired;
    Serial.printf("[NVS] Cap nhat trang thai pairing: %s\n", is_paired ? "PAIRED" : "UNPAIRED");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String incomingTopic = String(topic);
    Serial.printf("[MQTT] Nhan message tu topic: %s\n", topic);

    String responseTopic = "devices/" + cfg_plant_code + "/response";
    if (incomingTopic == responseTopic) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
            Serial.printf("[MQTT] Parse JSON response that bai: %s\n", error.c_str());
            return;
        }

        if (doc.containsKey("total_exp")) {
            cfg_total_exp = doc["total_exp"].as<int>();
        }
        if (doc.containsKey("rank_name")) {
            cfg_rank_name = doc["rank_name"].as<String>();
        }
        if (doc.containsKey("is_paired")) {
            bool paired = doc["is_paired"].as<bool>();
            if (paired != cfg_is_paired) {
                savePairingState(paired);
            }
        }
        Serial.printf("[MQTT] Cap nhat -> EXP: %d, Rank: %s\n", cfg_total_exp, cfg_rank_name.c_str());
        
        // Vẽ lại màn hình với Tu Vi & Cảnh Giới mới nhận được
        drawDashboard(0);
    }
}

// =============================================================
// NVS: Đọc cấu hình
// =============================================================
bool loadConfig() {
    prefs.begin("mocdao", true);
    cfg_ssid       = prefs.getString("ssid", "");
    cfg_pass       = prefs.getString("pass", "");
    cfg_verify_code = prefs.getString("verify_code", "");
    cfg_is_paired  = prefs.getBool("is_paired", false);
    prefs.end();

    // Dùng mã Plant Code đã nạp cứng vào firmware khi sản xuất
    cfg_plant_code = DEFAULT_PLANT_CODE;

    bool hasConfig = (cfg_ssid.length() > 0 && cfg_verify_code.length() >= 4);
    Serial.printf("[NVS] SSID='%s' PlantCode='%s' VerifyCode='%s' IsPaired=%d\n",
                  cfg_ssid.c_str(), cfg_plant_code.c_str(), cfg_verify_code.c_str(), cfg_is_paired);
    Serial.printf("[NVS] Config %s\n", hasConfig ? "OK" : "CHUA CO");
    return hasConfig;
}

// =============================================================
// NVS: Lưu cấu hình
// =============================================================
void saveConfig(const String& ssid, const String& pass, const String& verify_code) {
    prefs.begin("mocdao", false);
    prefs.putString("ssid",       ssid);
    prefs.putString("pass",       pass);
    prefs.putString("verify_code", verify_code);
    prefs.end();
    Serial.println("[NVS] Da luu config!");
}

// =============================================================
// NVS: Xoá toàn bộ cấu hình (factory reset)
// =============================================================
void clearConfig() {
    prefs.begin("mocdao", false);
    prefs.clear();
    prefs.end();
    Serial.println("[NVS] Da xoa config (factory reset)");
}

// =============================================================
// CONFIG MODE: HTML trang cấu hình
// =============================================================
const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Moc Dao Tu Tien - Cau hinh</title>
<style>
  body { font-family: sans-serif; background: #1a1a2e; color: #eee; padding: 20px; }
  h2   { color: #4ecca3; text-align: center; }
  p    { color: #aaa; text-align: center; font-size: 13px; }
  .box { background: #16213e; border-radius: 12px; padding: 24px;
         max-width: 400px; margin: 0 auto; }
  label  { display: block; margin: 12px 0 4px; color: #4ecca3; font-size: 14px; }
  input  { width: 100%; padding: 10px; border: 1px solid #333;
           background: #0f3460; color: #eee; border-radius: 8px;
           box-sizing: border-box; font-size: 14px; }
  button { width: 100%; padding: 12px; margin-top: 20px;
           background: #4ecca3; color: #1a1a2e; border: none;
           border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; }
  .note  { font-size: 12px; color: #888; margin-top: 4px; }
  .saved { color: #4ecca3; text-align: center; font-weight: bold; }
</style>
</head>
<body>
<div class="box">
  <h2>&#127807; Moc Dao Tu Tien</h2>
  <p>Cau hinh thiet bi IoT</p>
  <form action="/save" method="POST">
    <label>WiFi SSID (Ten mang)</label>
    <input name="ssid" type="text" placeholder="Ten WiFi nha ban" required>

    <label>WiFi Password</label>
    <input name="pass" type="password" placeholder="Mat khau WiFi">

    <label>Plant Code (Ma thiet bi)</label>
    <div style="padding: 10px; background: #0f3460; color: #4ecca3; border-radius: 8px; font-weight: bold; text-align: center;">{PLANT_CODE}</div>
           
    <label>Verify Code (Ma xac thuc)</label>
    <input name="verify_code" type="text" placeholder="VD: 123456"
           maxlength="16" required>
    <div class="note">Lay tu Admin Dashboard → Tao thiet bi</div>

    <button type="submit">Luu &amp; Khoi dong lai</button>
  </form>
</div>
</body>
</html>
)rawhtml";

const char SAVED_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta http-equiv="refresh" content="5; url=/">
<title>Da luu!</title>
<style>
  body { font-family: sans-serif; background: #1a1a2e; color: #eee;
         display: flex; align-items: center; justify-content: center;
         height: 100vh; margin: 0; }
  .box { text-align: center; }
  h2   { color: #4ecca3; }
</style>
</head>
<body>
<div class="box">
  <h2>&#10003; Da luu!</h2>
  <p>Thiet bi se khoi dong lai trong 3 giay...</p>
</div>
</body>
</html>
)rawhtml";

// =============================================================
// CONFIG MODE: Handlers web server
// =============================================================
void handleConfigRoot() {
    String html = FPSTR(CONFIG_HTML);
    html.replace("{PLANT_CODE}", DEFAULT_PLANT_CODE);
    configServer.send(200, "text/html", html);
}

void handleConfigSave() {
    String ssid       = configServer.arg("ssid");
    String pass       = configServer.arg("pass");
    String verify_code = configServer.arg("verify_code");

    // Validate cơ bản
    if (ssid.length() == 0 || verify_code.length() < 4) {
        configServer.send(400, "text/plain", "Thieu SSID hoac Verify Code!");
        return;
    }

    // Lưu NVS
    saveConfig(ssid, pass, verify_code);

    // Trả HTML xác nhận
    configServer.send_P(200, "text/html", SAVED_HTML);

    // Đợi 3 giây để browser nhận response xong rồi restart
    delay(3000);
    ESP.restart();
}

// =============================================================
// CONFIG MODE: Khởi động AP + Web Server
// =============================================================
void startConfigMode() {
    configMode = true;
    Serial.println("[Config] Bat che do cau hinh AP...");

    // Tên AP dạng "MocDao-ABCDEF" để phân biệt nhiều thiết bị
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[20];
    snprintf(apName, sizeof(apName), "MocDao-%02X%02X%02X", mac[3], mac[4], mac[5]);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, "");  // Không cần mật khẩu cho AP dễ truy cập
    delay(500);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[Config] AP: '%s' | IP: %s\n", apName, apIP.toString().c_str());

    dnsServer.start(53, "*", apIP); // DNS Server chuyển hướng mọi yêu cầu về IP của ESP32
    Serial.println("[Config] DNS Server da bat dau (DNS Redirect)");

    configServer.on("/",     HTTP_GET,  handleConfigRoot);
    configServer.on("/save", HTTP_POST, handleConfigSave);
    configServer.onNotFound([]() {
        // Redirect mọi request về trang config (Captive Portal)
        configServer.sendHeader("Location", "http://192.168.4.1/");
        configServer.send(302, "text/plain", "");
    });
    configServer.begin();
    Serial.println("[Config] Web server da bat dau");

    // Hiển thị OLED hướng dẫn
    oledClear();
    oled.setTextSize(1);
    oled.setCursor(0, 0);  oled.println("=== CAU HINH ===");
    oled.setCursor(0, 14); oled.printf("WiFi: %s", apName);
    oled.setCursor(0, 26); oled.println("Khong can mat khau");
    oled.setCursor(0, 38); oled.println("Mo trinh duyet:");
    oled.setCursor(0, 50); oled.println("192.168.4.1");
    oled.display();
}

// =============================================================
// MQTT: Tách IP/Host từ Server URL
// =============================================================
String getMqttHost(String url) {
    String host = url;
    int idx = host.indexOf("://");
    if (idx >= 0) {
        host = host.substring(idx + 3);
    }
    idx = host.indexOf(":");
    if (idx >= 0) {
        host = host.substring(0, idx);
    }
    idx = host.indexOf("/");
    if (idx >= 0) {
        host = host.substring(0, idx);
    }
    return host;
}



// =============================================================
// MQTT: Kết nối Broker song song (Non-blocking)
// =============================================================
void connectMQTT() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (mqttClient.connected()) return;

    unsigned long now = millis();
    if (now - lastMqttRetry < 10000) return; // Thử lại sau mỗi 10 giây
    lastMqttRetry = now;

    String host = getMqttHost(cfg_server);
    Serial.printf("[MQTT] Dang ket noi toi Broker: %s:%d...\n", host.c_str(), MQTT_PORT);

    mqttClient.setServer(host.c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    // BẮT BUỘC: Thêm số random vào Client ID để không bị Broker đá văng khi dùng Public Server
    String clientId = "MocDaoDevice-" + cfg_plant_code + "-" + String(random(1000, 9999));
    String statusTopic = "devices/" + cfg_plant_code + "/status";
    String responseTopic = "devices/" + cfg_plant_code + "/response";

    // LWT (Last Will and Testament) set to "offline"
    bool connected = mqttClient.connect(
        clientId.c_str(),
        NULL, NULL,
        statusTopic.c_str(),
        1, true,
        "offline"
    );

    if (connected) {
        Serial.println("[MQTT] KET NOI BROKER OK!");
        mqttClient.publish(statusTopic.c_str(), "online", true);
        mqttClient.subscribe(responseTopic.c_str(), 1);
        Serial.printf("[MQTT] Subscribed to response: %s\n", responseTopic.c_str());
    } else {
        Serial.printf("[MQTT] Ket noi failure, rc=%d\n", mqttClient.state());
    }
}

// =============================================================
// WiFi: Thử kết nối một lần
// =============================================================
bool tryConnectWiFi() {
    Serial.printf("[WiFi] Thu ket noi '%s'...\n", cfg_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg_ssid.c_str(), cfg_pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_RETRY_DELAY_MS) {
            Serial.println("[WiFi] Het gio!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] OK — IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

// =============================================================
// WiFi: Thử nhiều lần, nếu hết → Config Mode
// =============================================================
void connectWiFiOrConfig() {
    for (int attempt = 1; attempt <= WIFI_MAX_RETRY; attempt++) {
        Serial.printf("[WiFi] Lan thu %d/%d\n", attempt, WIFI_MAX_RETRY);

        char line2[32];
        snprintf(line2, sizeof(line2), "Thu %d/%d...", attempt, WIFI_MAX_RETRY);
        oledShow("Ket noi WiFi", line2, cfg_ssid.c_str());

        if (tryConnectWiFi()) {
            // Thành công
            char ipStr[20];
            WiFi.localIP().toString().toCharArray(ipStr, sizeof(ipStr));
            oledShow("WiFi OK!", ipStr, cfg_plant_code.c_str());
            delay(2000);
            return;
        }

        // Thất bại lần này
        WiFi.disconnect();
        if (attempt < WIFI_MAX_RETRY) {
            Serial.printf("[WiFi] That bai. Thu lai sau 3s...\n");
            oledShow("WiFi that bai!", "Thu lai sau 3s...", cfg_ssid.c_str());
            delay(3000);
        }
    }

    // Hết tất cả retry → Config Mode
    Serial.println("[WiFi] Khong the ket noi. Chuyen sang Config Mode.");
    oledShow("WiFi that bai!", "Vao che do", "cau hinh...");
    delay(2000);
    startConfigMode();
}

// =============================================================
// Cảm biến: Đọc tất cả (Bao gồm bộ lọc nhiễu Data Validation)
// =============================================================
SensorData readSensors() {
    SensorData data;
    data.isValid = true;
    data.errorMsg = "";

    // Đọc DHT22 thô
    float new_temp = dht.readTemperature();
    float new_hum  = dht.readHumidity();

    // 1. Kiểm tra Mất kết nối / Đứt dây (NaN) của DHT22
    if (isnan(new_temp) || isnan(new_hum)) { 
        data.isValid = false;
        data.errorMsg = "Loi: Mat DHT22!";
        data.temperature = -999.0;
        data.humidity = -999.0;
    } 
    // 2. Kiểm tra Ngưỡng Vật lý (Out of Range - Phi thực tế)
    else if (new_temp < -10.0 || new_temp > 60.0) {
        data.isValid = false;
        data.errorMsg = "Loi: Nhiet do ao!";
        data.temperature = new_temp;
        data.humidity = new_hum;
    }
    // 3. Kiểm tra Nhảy vọt đột ngột (Spike Check > 10 độ trong thời gian ngắn) -> Dữ liệu rác
    else if (lastSentData.isValid && abs(new_temp - lastSentData.temperature) > 10.0) {
        data.isValid = false;
        data.errorMsg = "Loi: Nhiet do rac!";
        data.temperature = new_temp;
        data.humidity = new_hum;
    }
    else {
        data.temperature = new_temp;
        data.humidity    = new_hum;
    }

    // 4. Đọc cảm biến độ ẩm đất
    int rawSoil = analogRead(PIN_SOIL_ADC);
    if (rawSoil == 0) {
        data.soilMoisture = -999.0;
        if (data.isValid) { 
            data.isValid = false;
            data.errorMsg = "Loi: Mat CB Dat!";
        }
    } else {
        data.soilMoisture = constrain(map(rawSoil, 4095, 1500, 0, 100), 0, 100);
    }

    // 5. Đọc cảm biến cường độ ánh sáng BH1750
    data.light = 0.0;
    if (bh1750Ok) {
        float lux = lightMeter.readLightLevel();
        if (lux >= 0.0) {
            data.light = lux;
        } else {
            data.light = -999.0;
            if (data.isValid) {
                data.isValid = false;
                data.errorMsg = "Loi: Mat BH1750!";
            }
        }
    } else {
        data.light = -999.0;
        // Cảm biến ánh sáng bị đứt ngay từ lúc khởi động
        if (data.isValid) {
            data.isValid = false;
            data.errorMsg = "Loi: Mat BH1750!";
        }
    }

    if (!data.isValid) {
        Serial.printf("[Sensor] INVALID: %s (T=%.1fC, H=%.1f%%, Soil=%.1f%%)\n", 
                      data.errorMsg.c_str(), data.temperature, data.humidity, data.soilMoisture);
    } else {
        Serial.printf("[Sensor] T=%.1fC H=%.1f%% Soil=%.1f%% L=%.0flux\n",
                    data.temperature, data.humidity, data.soilMoisture, data.light);
    }
    
    return data;
}

// =============================================================
// MQTT: Gửi telemetry
// =============================================================
void sendTelemetry() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT] Mat WiFi!");
        oledShow("Mat WiFi!", "Dang thu lai...");
        connectWiFiOrConfig();
        return;
    }

    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Chua ket noi Broker, bo qua gui.");
        oledShow("MQTT mat ket noi!", "Dang thu lai...");
        return;
    }

    // Đọc cảm biến
    SensorData sensors = readSensors();

    // Topic
    String telemetryTopic = "devices/" + cfg_plant_code + "/telemetry";

    // JSON payload đúng chuẩn backend: {"sensors": [{key, value}...]}
    StaticJsonDocument<400> doc;
    JsonArray arr = doc.createNestedArray("sensors");

    JsonObject s1 = arr.createNestedObject();
    s1["key"] = "temperature"; s1["value"] = sensors.temperature;

    JsonObject s2 = arr.createNestedObject();
    s2["key"] = "humidity"; s2["value"] = sensors.humidity;

    JsonObject s3 = arr.createNestedObject();
    s3["key"] = "soil_moisture"; s3["value"] = sensors.soilMoisture;

    JsonObject s4 = arr.createNestedObject();
    s4["key"] = "light"; s4["value"] = sensors.light;

    String payload;
    serializeJson(doc, payload);

    // MQTT Publish
    bool ok = mqttClient.publish(
        telemetryTopic.c_str(),
        payload.c_str(),
        false   // retain = false cho telemetry
    );

    if (ok) {
        Serial.printf("[MQTT] Telemetry sent → %s\n", telemetryTopic.c_str());
        Serial.printf("[MQTT] Payload: %s\n", payload.c_str());

        lastSensors = sensors;
        lastSentData = sensors; // Đánh dấu dữ liệu đã được gửi thành công để so sánh ở các vòng lặp sau
        drawDashboard(0);
    } else {
        Serial.println("[MQTT] Publish that bai!");
        oledShow("MQTT Publish loi!", telemetryTopic.c_str());
    }
}



// =============================================================
// SETUP
// =============================================================
void setup() {
    setCpuFrequencyMhz(80); // Hạ xung nhịp xuống 80MHz giúp giảm nhiệt độ đáng kể
    Serial.begin(115200);
    delay(1000); 
    Serial.println("\n\n[Boot] Moc Dao Tu Tien khoi dong...");

    // 1. Khoi tao I2C va OLED
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        oledOk = true;
        Serial.println("[OLED] Tim thay SSD1306 tai 0x3C");
    } else if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
        oledOk = true;
        Serial.println("[OLED] Tim thay SSD1306 tai 0x3D");
    }

    if (oledOk) {
        oledShow("Moc Dao Tu Tien", "v2.0 Khoi dong...");
    } else {
        Serial.println("[OLED] KHONG TIM THAY MAN HINH!");
    }

    // 2. Khoi tao DHT22
    dht.begin();

    // 3. Khoi tao BH1750 an toan
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        bh1750Ok = true;
        Serial.println("[BH1750] Tim thay cam bien anh sang!");
    } else {
        Serial.println("[BH1750] KHONG TIM THAY BH1750 - Bo qua.");
    }

    delay(1000);

#ifdef WOKWI_SIMULATION
    Serial.println("[SIM] WOKWI_SIMULATION mode");
    cfg_ssid       = WIFI_SSID;
    cfg_pass       = WIFI_PASS;
    cfg_plant_code = DEFAULT_PLANT_CODE;
    cfg_verify_code = DEFAULT_VERIFY_CODE;
    cfg_server     = SERVER_HOST;
    oledShow("WOKWI SIM MODE", cfg_plant_code.c_str(), "Dang ket noi WiFi...");
    connectWiFiOrConfig();
    if (WiFi.status() == WL_CONNECTED) {
        lastSensors = readSensors();
    }
#else
    bool hasConfig = loadConfig();
    if (!hasConfig) {
        Serial.println("[Boot] Chua co config -> Config Mode");
        oledShow("Chua co cau hinh!", "Vao Config Mode...");
        delay(2000);
        startConfigMode();
    } else {
        connectWiFiOrConfig();
        if (WiFi.status() == WL_CONNECTED) {
            lastSensors = readSensors();
        }
    }
#endif
}

// =============================================================
// LOOP
// =============================================================
void loop() {
    if (configMode) {
        dnsServer.processNextRequest(); // Xử lý các yêu cầu chuyển hướng DNS từ Captive Portal
        configServer.handleClient();
        return;
    }

    // MQTT
    connectMQTT();
    mqttClient.loop();

    unsigned long now = millis();

    // Tự động chuyển đổi trang màn hình OLED mỗi 4 giây
    if (now - lastScreenSwitch >= SCREEN_SWITCH_INTERVAL_MS || lastScreenSwitch == 0) {
        lastScreenSwitch = now;
        currentScreen = (currentScreen + 1) % 2;
        drawDashboard(0);
    }

    // 1. Đọc cảm biến định kỳ mỗi 5 giây
    if (now - lastSampleTime >= 5000 || lastSampleTime == 0) {
        lastSampleTime = now;
        SensorData current = readSensors();
        lastSensors = current;

        // Nếu dữ liệu không hợp lệ -> Hiện cảnh báo trên Serial & vẽ ERR lên OLED, KHÔNG gửi lên Server
        if (!current.isValid) {
            Serial.printf("[Loop] LOI CAM BIEN: %s - Tam dung gui telemetry.\n", current.errorMsg.c_str());
            drawDashboard(0);
            return; 
        }

        // 2. Kiểm tra thay đổi so với lần gửi thành công gần nhất
        bool hasSignificantChange = false;
        if (!lastSentData.isValid) {
            hasSignificantChange = true; 
        } else {
            if (abs(current.temperature - lastSentData.temperature) >= THRESHOLD_TEMP) hasSignificantChange = true;
            if (abs(current.humidity - lastSentData.humidity) >= THRESHOLD_HUM)       hasSignificantChange = true;
            if (abs(current.soilMoisture - lastSentData.soilMoisture) >= THRESHOLD_SOIL) hasSignificantChange = true;
            if (abs(current.light - lastSentData.light) >= THRESHOLD_LIGHT)           hasSignificantChange = true;
        }

        // 3. Chỉ gửi telemetry khi có thay đổi chỉ số cảm biến (loại bỏ chu kỳ 60s và heartbeat 10 phút)
        if (hasSignificantChange) {
            Serial.println("[Loop] Co thay doi chi so -> Gui MQTT Telemetry");
            lastSendTime = now;
            sendTelemetry();
        } else {
            // Cập nhật dashboard oled thường trực
            drawDashboard(0);
        }
    }
}
