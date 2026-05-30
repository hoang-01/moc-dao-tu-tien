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
 *   → POST /api/devices/{plant_code}/telemetry
 *   → Hiển thị OLED
 *
 * Thư viện (platformio.ini):
 *   - ArduinoJson, DHT, Adafruit TSL2561, Adafruit SSD1306
 *   - WebServer (built-in ESP32), Preferences (built-in ESP32)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include "secrets.h"

// ── MQTT ──────────────────────────────────────────────────────
#define MQTT_PORT             1883
WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastMqttRetry   = 0;
bool          isWatering      = false;
unsigned long waterStartTime   = 0;
#define WATER_DURATION_MS     5000 // Thời gian tưới mặc định (5s)

// ── Thời gian ─────────────────────────────────────────────────
#define TELEMETRY_INTERVAL_MS  6000UL   // 60 giây
#define WIFI_MAX_RETRY         3          // Số lần thử kết nối WiFi
#define WIFI_RETRY_DELAY_MS    10000      // Mỗi lần thử đợi tối đa 10s
#define CONFIG_TIMEOUT_MS      300000UL  // Portal tự đóng sau 5 phút nếu không có ai vào

// ── OLED ─────────────────────────────────────────────────────
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oledOk = false;

// ── Phần cứng ────────────────────────────────────────────────
DHT dht(PIN_DHT22, DHT22);
Adafruit_TSL2561_Unified tsl(TSL2561_ADDR_FLOAT, 12345);

// ── NVS ───────────────────────────────────────────────────────
Preferences prefs;

// ── Config Portal Web Server ──────────────────────────────────
WebServer configServer(80);

// ── Biến cấu hình (load từ NVS) ───────────────────────────────
String cfg_ssid       = "";
String cfg_pass       = "";
String cfg_plant_code = "";
String cfg_server     = SERVER_HOST; // Sử dụng SERVER_HOST định nghĩa trong secrets.h

// ── Biến trạng thái ───────────────────────────────────────────
unsigned long lastSendTime      = 0;
bool          configMode        = false;

// ── Cấu trúc dữ liệu ─────────────────────────────────────────
struct SensorData {
    float temperature;
    float humidity;
    float soilMoisture;
    float light;
};

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
// NVS: Đọc cấu hình
// =============================================================
bool loadConfig() {
    prefs.begin("mocdao", true);
    cfg_ssid       = prefs.getString("ssid", "");
    cfg_pass       = prefs.getString("pass", "");
    cfg_plant_code = prefs.getString("plant_code", "");
    prefs.end();

    bool hasConfig = (cfg_ssid.length() > 0 && cfg_plant_code.length() >= 6);
    Serial.printf("[NVS] SSID='%s' PlantCode='%s'\n",
                  cfg_ssid.c_str(), cfg_plant_code.c_str());
    Serial.printf("[NVS] Config %s\n", hasConfig ? "OK" : "CHUA CO");
    return hasConfig;
}

// =============================================================
// NVS: Lưu cấu hình
// =============================================================
void saveConfig(const String& ssid, const String& pass,
                const String& plant_code) {
    prefs.begin("mocdao", false);
    prefs.putString("ssid",       ssid);
    prefs.putString("pass",       pass);
    prefs.putString("plant_code", plant_code);
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

    <label>Plant Code</label>
    <input name="plant_code" type="text" placeholder="VD: A7B3F2K9"
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
    configServer.send_P(200, "text/html", CONFIG_HTML);
}

void handleConfigSave() {
    String ssid       = configServer.arg("ssid");
    String pass       = configServer.arg("pass");
    String plant_code = configServer.arg("plant_code");

    // Validate cơ bản
    if (ssid.length() == 0 || plant_code.length() < 6) {
        configServer.send(400, "text/plain", "Thieu SSID hoac Plant Code!");
        return;
    }

    // Lưu NVS
    saveConfig(ssid, pass, plant_code);

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
// MQTT: Callback nhận tín hiệu từ Broker
// =============================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[MQTT] Nhan tin tu topic: %s\n", topic);
    
    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.printf("[MQTT] Payload: %s\n", msg.c_str());

    // Parse JSON command
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    String command = "";
    int duration = WATER_DURATION_MS / 1000;

    if (!error) {
        command = doc["command"].as<String>();
        if (doc.containsKey("duration")) {
            duration = doc["duration"].as<int>();
        }
    } else {
        command = msg;
    }

    if (command == "water" || command == "tuoi_nuoc") {
        Serial.printf("[MQTT] KICH HOAT VOI NUOC TRONG %d GIAY!\n", duration);
        isWatering = true;
        waterStartTime = millis();

        // OLED feedback
        oledClear();
        oled.setTextSize(2);
        oled.setCursor(0, 4);
        oled.println("💧 DANG TUOI");
        oled.setTextSize(1);
        oled.setCursor(0, 26);
        oled.printf("Moc Dao: %s\n", cfg_plant_code.c_str());
        oled.setCursor(0, 38);
        oled.printf("Thoi gian: %ds\n", duration);
        oled.setCursor(0, 50);
        oled.println("Relay: [ ON ]");
        oled.display();
    }
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

    String clientId = "MocDaoDevice-" + cfg_plant_code;
    String statusTopic = "devices/" + cfg_plant_code + "/status";
    String controlTopic = "devices/" + cfg_plant_code + "/control";

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
        mqttClient.subscribe(controlTopic.c_str(), 1);
        Serial.printf("[MQTT] Subscribed: %s\n", controlTopic.c_str());
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
// Cảm biến: Đọc tất cả
// =============================================================
SensorData readSensors() {
    SensorData data;

    // DHT22
    data.temperature = dht.readTemperature();
    data.humidity    = dht.readHumidity();
    if (isnan(data.temperature)) { data.temperature = 0.0; }
    if (isnan(data.humidity))    { data.humidity    = 0.0; }

    // Soil Moisture (ADC → %)
    // ADC 4095 = khô hoàn toàn (0%), ADC 1500 = ướt (100%)
    int rawSoil = analogRead(PIN_SOIL_ADC);
    data.soilMoisture = constrain(map(rawSoil, 4095, 1500, 0, 100), 0, 100);

    // TSL2561 (Lux)
    sensors_event_t event;
    tsl.getEvent(&event);
    data.light = (event.light > 0) ? event.light : 0.0;

    Serial.printf("[Sensor] T=%.1fC H=%.1f%% Soil=%.1f%% L=%.0flux\n",
                  data.temperature, data.humidity, data.soilMoisture, data.light);
    return data;
}

// =============================================================
// HTTP: Gửi telemetry
// =============================================================
void sendTelemetry() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Mat WiFi!");
        oledShow("Mat WiFi!", "Dang thu lai...");
        connectWiFiOrConfig();
        return;
    }

    // Đọc cảm biến
    SensorData sensors = readSensors();

    // Xây URL
    String url = cfg_server + "/api/devices/" + cfg_plant_code + "/telemetry";

    // JSON payload đúng chuẩn backend
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

    // POST request
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Plant-Code", cfg_plant_code);
    http.setTimeout(10000);

    int code = http.POST(payload);
    Serial.printf("[HTTP] %d | URL: %s\n", code, url.c_str());

    if (code == 200) {
        String body = http.getString();
        StaticJsonDocument<256> resp;
        if (!deserializeJson(resp, body)) {
            bool   expAwarded = resp["exp_awarded"].as<bool>();
            String message    = resp["message"].as<String>();

            // Trích quality từ message: "Xử lý thành công. Chất lượng: GOOD"
            String quality = "FAIR";
            int idx = message.indexOf("Chất lượng: ");
            if (idx < 0) idx = message.indexOf("Chat luong: ");
            if (idx >= 0) {
                quality = message.substring(idx + 12);
                quality.trim();
            }

            Serial.printf("[HTTP] OK | quality=%s exp=%d\n",
                          quality.c_str(), expAwarded);

            // Hiển thị OLED kết quả
            oledClear();
            oled.setTextSize(2);
            oled.setCursor(0, 0);
            oled.println(quality);           // GOOD / EXCELLENT...

            oled.setTextSize(1);
            oled.setCursor(0, 22);
            oled.println(expAwarded ? "+Tu Vi!" : "Anti-spam...");

            oled.setCursor(0, 36);
            char buf[22];
            snprintf(buf, sizeof(buf), "T:%.0fC H:%.0f%%",
                     sensors.temperature, sensors.humidity);
            oled.println(buf);

            oled.setCursor(0, 48);
            snprintf(buf, sizeof(buf), "Dat:%.0f%% L:%.0flux",
                     sensors.soilMoisture, sensors.light);
            oled.println(buf);
            oled.display();
        }
    } else if (code == 403) {
        Serial.println("[HTTP] 403: Plant Code sai!");
        oledShow("Loi 403!", "Plant Code sai!", cfg_plant_code.c_str(),
                 "Vao 192.168.4.1");
    } else if (code == 400) {
        Serial.println("[HTTP] 400: " + http.getString());
        oledShow("Loi 400!", "Data khong hop le");
    } else {
        String errStr = http.errorToString(code);
        Serial.printf("[HTTP] Loi (%d): %s\n", code, errStr.c_str());
        oledShow("Loi ket noi!", ("Code: " + String(code)).c_str(), errStr.c_str());
    }

    http.end();
}

// =============================================================
// SETUP
// =============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n[Boot] Moc Dao Tu Tien v2.0 khoi dong...");

    // Khởi tạo I2C và OLED
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        oledOk = true;
        oledShow("Moc Dao Tu Tien", "v2.0 Khoi dong...");
    } else {
        Serial.println("[OLED] Khong tim thay man hinh!");
    }

    // Khởi tạo DHT22
    dht.begin();

    // Khởi tạo TSL2561
    if (tsl.begin()) {
        tsl.enableAutoRange(true);
        tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
    } else {
        Serial.println("[TSL] Khong tim thay cam bien anh sang!");
    }

    delay(1000);

    // Đọc cấu hình từ NVS
    bool hasConfig = loadConfig();

    if (!hasConfig) {
        // Lần đầu dùng hoặc chưa cấu hình → Config Mode luôn
        Serial.println("[Boot] Chua co config → Config Mode");
        oledShow("Chua co cau hinh!", "Vao Config Mode...");
        delay(2000);
        startConfigMode();
    } else {
        // Có config → thử kết nối WiFi
        connectWiFiOrConfig();
    }
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

    // ── Xử lý tưới nước bất đồng bộ ───────────────────────────
    if (isWatering) {
        if (millis() - waterStartTime >= WATER_DURATION_MS) {
            isWatering = false;
            Serial.println("[MQTT] TUOI NUOC HOAN THANH!");
            // Quay lại màn hình telemetry ở vòng lặp sau
        } else {
            // Dừng hiển thị đếm ngược khi đang tưới nước
            delay(100);
            return;
        }
    }

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
                oledClear();
                oled.setTextSize(1);
                oled.setCursor(0, 0);  oled.println("Moc Dao Tu Tien");
                oled.setCursor(0, 14); oled.printf("Code: %s", cfg_plant_code.c_str());
                oled.setCursor(0, 28); oled.printf("Cho: %lus...", remain);
                oled.setCursor(0, 42); oled.printf("WiFi: %s",
                    WiFi.status() == WL_CONNECTED ? "OK" : "MAT");
                oled.display();
            }
        }
        delay(100);
        return;
    }

    // Đến lúc gửi rồi
    lastSendTime = now;
    sendTelemetry();
}
