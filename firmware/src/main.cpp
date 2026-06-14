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
 *   Đọc DHT22 → Đọc Soil → Đọc BH1750
 *   → MQTT Publish: devices/{plant_code}/telemetry
 *   → Hiển thị OLED
 *
 * Thư viện (platformio.ini):
 *   - ArduinoJson, DHT, BH1750, Adafruit SSD1306
 *   - WebServer (built-in ESP32), Preferences (built-in ESP32)
 *   - PubSubClient (MQTT)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
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
#ifndef SOIL_DRY_VALUE
#define SOIL_DRY_VALUE         4095
#endif
#ifndef SOIL_WET_VALUE
#define SOIL_WET_VALUE         1500
#endif
#define SENSOR_MIN_READ_INTERVAL_MS 2000UL

// ── OLED ─────────────────────────────────────────────────────
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oledOk = false;
bool lightOk = false;

// ── Phần cứng ────────────────────────────────────────────────
DHT dht(PIN_DHT22, DHT22);
BH1750 lightMeter;

// ── NVS ───────────────────────────────────────────────────────
Preferences prefs;

// ── Config Portal Web Server ──────────────────────────────────
WebServer configServer(80);

// ── Biến cấu hình (load từ NVS) ───────────────────────────────
String cfg_ssid       = "";
String cfg_pass       = "";
String cfg_plant_code = "";
String cfg_verify_code = "";
String cfg_server     = SERVER_HOST; // Sử dụng SERVER_HOST định nghĩa trong secrets.h

// ── Biến trạng thái ───────────────────────────────────────────
unsigned long lastSendTime      = 0;
unsigned long lastSensorReadAt  = 0;
bool          configMode        = false;

// ── Cấu trúc dữ liệu ─────────────────────────────────────────
struct SensorData {
    float temperature;
    float humidity;
    float soilMoisture;
    float light;
};

// ── Gamification ──────────────────────────────────────────────
int           cfg_total_exp = 0;
String        cfg_rank_name = "Pham Moc";
SensorData    lastSensors   = {-999.0, -999.0, -999.0, -999.0};

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
    Serial.printf("oledOk=%d\n", oledOk);
    if (!oledOk) return;
    oledClear();
    oled.setCursor(0, 0);  oled.println(line1);
    oled.setCursor(0, 16); oled.println(line2);
    oled.setCursor(0, 32); oled.println(line3);
    oled.setCursor(0, 48); oled.println(line4);
    oled.println("TEST");
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
// Logic Gamification: Lấy Trạng Thái Linh Khí
// =============================================================
String getLinhKhiState(float soil, float temp) {
    // Xử lý các mã lỗi phần cứng từ readSensors()
    if (temp == -999.0) return "Loi: Mat ket noi DHT!"; 
    if (temp == -998.0) return "Loi: DHT Du lieu rac!";
    if (temp == -997.0) return "Loi: Nhiet do nhay vot!";
    if (soil == -999.0) return "Loi: Cam bien dat!";

    if (temp > 40.0 || soil <= 5.0) return "Tau hoa nhap ma!"; // DANGER
    if (soil >= 60.0) return "Linh khi doi dao"; // EXCELLENT
    if (soil >= 40.0) return "Dang phat trien"; // GOOD
    if (soil >= 20.0) return "Tinh lang tu luyen"; // FAIR
    return "Linh khi suy kiet"; // POOR
}

const char* getOledLinhKhiState(float soil, float temp) {
    if (temp == -999.0) return "DHT LOST";
    if (temp == -998.0) return "DHT BAD";
    if (temp == -997.0) return "TEMP SPIKE";
    if (soil == -999.0) return "SOIL ERR";

    if (temp > 40.0 || soil <= 5.0) return "DANGER";
    if (soil >= 60.0) return "GOOD";
    if (soil >= 40.0) return "NORMAL";
    if (soil >= 20.0) return "LOW";
    return "DRY";
}

String truncateForOled(const String& value, size_t maxLen) {
    if (value.length() <= maxLen) return value;
    return value.substring(0, maxLen);
}

const char* i2cErrorName(uint8_t error) {
    switch (error) {
        case 0: return "OK";
        case 1: return "DATA_TOO_LONG";
        case 2: return "ADDR_NACK";
        case 3: return "DATA_NACK";
        case 4: return "OTHER";
        case 5: return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

void logI2CBusState(const char* label) {
    int sda = digitalRead(PIN_OLED_SDA);
    int scl = digitalRead(PIN_OLED_SCL);
    Serial.printf("[I2C] %s | SDA(GPIO%d)=%s SCL(GPIO%d)=%s\n",
                  label,
                  PIN_OLED_SDA, sda == HIGH ? "HIGH" : "LOW",
                  PIN_OLED_SCL, scl == HIGH ? "HIGH" : "LOW");
}

bool i2cDevicePresent(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    Serial.printf("[I2C] Probe 0x%02X -> err=%u (%s)\n", address, error, i2cErrorName(error));
    return error == 0;
}

void scanI2CDevices() {
    Serial.println("[I2C] Dang scan thiet bi...");
    logI2CBusState("Truoc scan");
    bool found = false;

    const uint8_t addresses[] = {0x3C, 0x23, 0x5C};
    for (uint8_t address : addresses) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("[I2C] Tim thay dia chi 0x%02X\n", address);
            found = true;
        } else {
            Serial.printf("[I2C] Khong thay 0x%02X (err=%u %s)\n",
                          address, error, i2cErrorName(error));
        }
        delay(5);
    }

    if (!found) {
        Serial.println("[I2C] Khong tim thay OLED/BH1750 tren bus");
    }
    logI2CBusState("Sau scan");
}

bool beginLightSensor() {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
        Serial.println("[BH1750] Tim thay cam bien anh sang tai 0x23");
        return true;
    }

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
        Serial.println("[BH1750] Tim thay cam bien anh sang tai 0x5C");
        return true;
    }

    return false;
}

// =============================================================
// OLED: Vẽ màn hình Dashboard (Tu Vi + Cảnh Giới)
// =============================================================
void drawDashboard(unsigned long remainSecs) {
    if (!oledOk) return;
    oledClear();
    oled.setTextSize(1);
    oled.setTextWrap(false); // Ép không cho tự động rớt chữ M xuống dòng
    
    // Dòng 1: Code và icon WiFi/MQTT
    oled.setCursor(0, 0);
    String plant = truncateForOled(cfg_plant_code, 6);
    String verify = truncateForOled(cfg_verify_code, 4);
    oled.printf("P:%s V:%s", plant.c_str(), verify.c_str());
    
    // Icon WiFi và MQTT ở góc phải (Chỉ dùng 1 ký tự W và M)
    if (WiFi.status() == WL_CONNECTED) {
        oled.setCursor(110, 0); oled.print("W");
    }
    if (mqttClient.connected()) {
        oled.setCursor(120, 0); oled.print("M");
    }
    
    // Dòng 2: Chỉ số cảm biến (chia 2 cột nhỏ)
    char buf[32];
    oled.setCursor(0, 11);
    if (lastSensors.temperature <= -997.0) {
        snprintf(buf, sizeof(buf), "T: ERR  H: ERR");
    } else {
        snprintf(buf, sizeof(buf), "T:%.1fC H:%.1f%%", lastSensors.temperature, lastSensors.humidity);
    }
    oled.println(buf);
    
    oled.setCursor(0, 21);
    if (lastSensors.soilMoisture == -999.0) {
        snprintf(buf, sizeof(buf), "Dat: ERR L:%.0flx", lastSensors.light);
    } else {
        snprintf(buf, sizeof(buf), "Dat:%.0f%% L:%.0flx", lastSensors.soilMoisture, lastSensors.light);
    }
    oled.println(buf);
    
    // Dòng 3: Trạng Thái Linh Khí
    oled.setCursor(0, 33);
    snprintf(buf, sizeof(buf), "TT:%s", getOledLinhKhiState(lastSensors.soilMoisture, lastSensors.temperature));
    oled.println(buf);
    
    // Dòng 4: Cảnh Giới và EXP
    oled.setCursor(0, 45);
    String rank = truncateForOled(cfg_rank_name, 10);
    snprintf(buf, sizeof(buf), "%s:%d", rank.c_str(), cfg_total_exp);
    oled.println(buf);
    
    oled.display();
}

// =============================================================
// MQTT: Callback nhận kết quả Tu Vi/Cảnh Giới từ Broker
// =============================================================
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
    prefs.end();

    // Dùng mã Plant Code đã nạp cứng vào firmware khi sản xuất
    cfg_plant_code = DEFAULT_PLANT_CODE;

    bool hasConfig = (cfg_ssid.length() > 0 && cfg_verify_code.length() >= 4);
    Serial.printf("[NVS] SSID='%s' PlantCode='%s' VerifyCode='%s'\n",
                  cfg_ssid.c_str(), cfg_plant_code.c_str(), cfg_verify_code.c_str());
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
    unsigned long now = millis();
    if (lastSensorReadAt > 0 && now - lastSensorReadAt < SENSOR_MIN_READ_INTERVAL_MS) {
        Serial.println("[Sensor] Dung cache, tranh doc DHT qua day");
        return lastSensors;
    }
    lastSensorReadAt = now;

    SensorData data;

    // Đọc DHT22 thô
    float new_temp = dht.readTemperature();
    float new_hum  = dht.readHumidity();

    // 1. Kiểm tra Mất kết nối / Đứt dây (NaN) -> Test Case 11
    if (isnan(new_temp) || isnan(new_hum)) { 
        data.temperature = -999.0; 
        data.humidity    = -999.0;
    } 
    // 2. Kiểm tra Ngưỡng Vật lý (Out of Range) -> Test Case 7
    else if (new_temp < -20.0 || new_temp > 80.0 || new_hum < 0.0 || new_hum > 100.0) {
        data.temperature = -998.0; 
        data.humidity    = -998.0;
    }
    // 3. Kiểm tra Nhảy vọt (Data Spike - Variance Check > 15 độ) -> Test Case 10
    else if (lastSensors.temperature > -900.0 && abs(new_temp - lastSensors.temperature) > 15.0) {
        data.temperature = -997.0; 
        data.humidity    = new_hum;
    }
    // Dữ liệu Sạch (Clean Data)
    else {
        data.temperature = new_temp;
        data.humidity    = new_hum;
    }

    // Soil Moisture (ADC → %)
    int rawSoil = 0;
    int soilMin = 4095;
    int soilMax = 0;
    int soilZeroCount = 0;
    long soilSum = 0;
    const int soilSamples = 8;

    for (int i = 0; i < soilSamples; i++) {
        int sample = analogRead(PIN_SOIL_ADC);
        if (sample == 0) soilZeroCount++;
        soilMin = min(soilMin, sample);
        soilMax = max(soilMax, sample);
        soilSum += sample;
        delay(2);
    }
    rawSoil = soilSum / soilSamples;

    // ADC ESP32 trả về 0 nếu chân bị chạm GND (ngắn mạch) hoặc hỏng hoàn toàn
    if (soilZeroCount == soilSamples) {
        data.soilMoisture = -999.0; // Lỗi cảm biến
    } else {
        data.soilMoisture = constrain(map(rawSoil, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100), 0, 100);
    }
    Serial.printf("[Soil] samples=%d avg=%d min=%d max=%d zero=%d/%d cal(dry=%d wet=%d)\n",
                  soilSamples, rawSoil, soilMin, soilMax, soilZeroCount, soilSamples,
                  SOIL_DRY_VALUE, SOIL_WET_VALUE);

    // BH1750 (Lux)
    if (lightOk) {
        float lux = lightMeter.readLightLevel();
        data.light = (lux >= 0) ? lux : 0.0;
    } else {
        data.light = 0.0;
    }

    Serial.printf("[Sensor] T=%.1fC H=%.1f%% Soil=%.1f%% (Raw=%d) L=%.0flux (BH1750=%s)\n",
                  data.temperature, data.humidity, data.soilMoisture, rawSoil, data.light, lightOk ? "OK" : "ERR");
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
    lastSensors = sensors;

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

        drawDashboard(0);
    } else {
        Serial.println("[MQTT] Publish that bai!");
        oledShow("MQTT Publish loi!", telemetryTopic.c_str());
    }
}



#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =============================================================
// SETUP
// =============================================================
void setup() {
    // 1. Tắt tính năng báo lỗi sụt nguồn (Brownout) để tránh mạch bị sập liên tục
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // 2. Tắt hẳn WiFi ngay khi khởi động để tránh IC WiFi rút dòng quá mạnh (gây tụt áp)
    WiFi.mode(WIFI_OFF);
    delay(500);

    Serial.begin(115200);
    delay(1000); // Chờ 1 giây để đảm bảo Serial Terminal của Wokwi bắt kịp mạch
    Serial.println("\n\n[Boot] Moc Dao Tu Tien khoi dong...");

    // Khởi tạo I2C và OLED
    logI2CBusState("Truoc Wire.begin");
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.setTimeOut(1000);
    logI2CBusState("Sau Wire.begin");

    // Khởi tạo BH1750 TRƯỚC màn hình OLED để tránh OLED ép xung I2C làm treo bus
    if (beginLightSensor()) {
        lightOk = true;
    } else {
        Serial.println("[BH1750] Khong tim thay cam bien anh sang!");
    }

    // Sau đó mới khởi tạo OLED
    if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false)) {
        oledOk = true;
        oledShow("Moc Dao Tu Tien", "v2.0 Khoi dong...");
        Serial.println("[OLED] Khoi tao OK tai 0x3C");
    } else {
        Serial.println("[OLED] Khong tim thay man hinh!");
    }
    logI2CBusState("Sau OLED.begin");

    // Khởi tạo DHT22
    dht.begin();

    // Khởi tạo ADC cảm biến đất
    pinMode(PIN_SOIL_ADC, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_SOIL_ADC, ADC_11db);

    // Đã chuyển khởi tạo BH1750 lên trên
    delay(1000);


}

// =============================================================
// LOOP
// =============================================================
void loop() {
    if (configMode) {
        // Đang ở Config Mode → chỉ handle web requests
        configServer.handleClient();
        return;
    }

    // ── MQTT Connection maintenance ───────────────────────────
    connectMQTT();
    mqttClient.loop();

    // ── Telemetry Mode ────────────────────────────────────────
    unsigned long now = millis();

    // Chưa đến 60 giây → hiển thị đồng hồ đếm ngược
    if (now - lastSendTime < TELEMETRY_INTERVAL_MS) {
        if (oledOk) {
            unsigned long remain = (TELEMETRY_INTERVAL_MS - (now - lastSendTime)) / 1000;

            // Chỉ cập nhật OLED mỗi giây để tránh nhấp nháy
            static unsigned long lastOledUpdate = 0;
            if (now - lastOledUpdate >= 1000) {
                lastOledUpdate = now;
                
                // Cập nhật cảm biến định kỳ mỗi 2 giây để hiển thị thời gian thực
                static unsigned long lastSensorReadTime = 0;
                if (now - lastSensorReadTime >= 2000) {
                    lastSensorReadTime = now;
                    lastSensors = readSensors();
                }

                drawDashboard(remain);
            }
        }
        delay(100);
        return;
    }

    // Đến lúc gửi rồi
    lastSendTime = now;
    sendTelemetry();
}
