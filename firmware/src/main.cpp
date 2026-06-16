#include "secrets.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <HTTPClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(PIN_DHT22, DHT22);
BH1750 lightMeter;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Timers
unsigned long lastSensorRead = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastMQTTPublish = 0;
unsigned long lastReconnectAttempt =
    0xFFFFFFFF - 5000; // Để nó connect ngay lập tức lần đầu

// Data từ Server
long total_exp = 0;
String rank_name = "Chưa rõ";
String currentStatus = "WAIT";
String serverMessage = "";

// Sensor State
float lastSentTemp = -999.0;
float lastSentHum = -999.0;
bool lastDHTError = false;

float currentTemp = 0.0;
float currentHum = 0.0;
float currentLux = 0.0;
int currentSoil = 0;
bool currentDHTError = false;

// Topics
char telemetry_topic[100];
char status_topic[100];
char response_topic[100];

// JWT Token
String jwtToken = "";

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // --- Dòng 1: P:{plant_code}  WM ---
  display.setCursor(0, 0);
  display.printf("P:%s", String(DEFAULT_PLANT_CODE).substring(0, 6).c_str());

  // Vẽ W và M ở góc phải
  display.setCursor(128 - 12, 0);
  if (WiFi.status() == WL_CONNECTED)
    display.print("W");
  else
    display.print(" ");
  if (mqttClient.connected())
    display.print("M");
  else
    display.print(" ");

  // --- Dòng 2: Trạng thái ---
  // Thu nhỏ chữ và dồn lên trên để tránh nửa dưới bị nhiễu
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.printf("Trang thai: %s", currentStatus.c_str());

  // --- Dòng 3: Cấp bậc và EXP ---
  display.setCursor(0, 20);
  String display_rank = rank_name;
  if (rank_name == "Luyện Khí")
    display_rank = "Luyen Khi";
  else if (rank_name == "Trúc Cơ")
    display_rank = "Truc Co";
  else if (rank_name == "Kim Đan")
    display_rank = "Kim Dan";
  else if (rank_name == "Chưa rõ")
    display_rank = "Chua ro";

  display.printf("%s: %ld", display_rank.c_str(), total_exp);

  display.display();
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == String(response_topic)) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {

      if (doc.containsKey("total_exp")) {
        total_exp = doc["total_exp"].as<long>();
      }

      if (doc.containsKey("rank_name")) {
        rank_name = doc["rank_name"].as<String>();
      }

      if (doc.containsKey("status")) {
        currentStatus = doc["status"].as<String>();
      }

      if (doc.containsKey("message")) {
        serverMessage = doc["message"].as<String>();
      }

      Serial.println("=== SERVER UPDATE ===");
      Serial.printf("EXP    : %ld\n", total_exp);
      Serial.printf("Rank   : %s\n", rank_name.c_str());
      Serial.printf("Status : %s\n", currentStatus.c_str());

      updateOLED();
    }
  }
}

void reconnect() {
  if (mqttClient.connected())
    return;

  unsigned long now = millis();
  if (now - lastReconnectAttempt > 5000) {
    lastReconnectAttempt = now;
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, status_topic,
                           0, true, "offline")) {
      Serial.println("connected");
      mqttClient.publish(status_topic, "online", true);
      mqttClient.subscribe(response_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    // Ép tốc độ I2C về chuẩn 100kHz để tránh nghẽn/xung đột với BH1750
    Wire.setClock(100000);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("Moc Dao Tu Tien");
    display.println("Khoi dong...");
    display.display();
  }

  // Khởi tạo Cảm biến
  dht.begin();
  
  // Mở bus I2C riêng (Wire1) cho BH1750: SDA = 25, SCL = 26
  Wire1.begin(25, 26); 
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1)) {
    Serial.println(F("BH1750 initialized"));
  } else {
    Serial.println(F("Error initializing BH1750, check wiring/address!"));
  }

  pinMode(PIN_SOIL_ADC, INPUT);

  // Setup Topics
  snprintf(telemetry_topic, sizeof(telemetry_topic), "devices/%s/telemetry",
           DEFAULT_PLANT_CODE);
  snprintf(status_topic, sizeof(status_topic), "devices/%s/status",
           DEFAULT_PLANT_CODE);
  snprintf(response_topic, sizeof(response_topic), "devices/%s/response",
           DEFAULT_PLANT_CODE);

  // Setup WiFiManager
  WiFiManager wm;
  // Chỉ hiển thị đúng nút cấu hình WiFi, ẩn hết các tính năng update/info khác
  std::vector<const char *> menu = {"wifi"};
  wm.setMenu(menu);

  String apName = "MocDao-" + String(DEFAULT_PLANT_CODE).substring(0, 6);
  bool res = wm.autoConnect(apName.c_str());

  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("WiFi connected!");

    // Thực hiện HTTP Auth lấy Token
    HTTPClient http;
    String authUrl = String(SERVER_HOST) + "/api/devices/" + DEFAULT_PLANT_CODE + "/auth";
    Serial.print("Authenticating with ");
    Serial.println(authUrl);
    
    http.begin(authUrl);
    http.addHeader("Content-Type", "application/json");
    
    String payload = "{\"verify_code\":\"" + String(DEFAULT_VERIFY_CODE) + "\"}";
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode == 200) {
      String response = http.getString();
      JsonDocument doc;
      deserializeJson(doc, response);
      jwtToken = doc["token"].as<String>();
      Serial.println("Auth Success! Token acquired.");
    } else {
      Serial.printf("Auth Failed! Code: %d\n", httpResponseCode);
      Serial.println(http.getString());
    }
    http.end();
  }

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  // Khởi tạo để gửi bản tin ngay sau khi boot
  lastMQTTPublish = millis() - 300000UL;
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnect();
    } else {
      mqttClient.loop();
    }
  }

  unsigned long now = millis();

  // 1. OLED REFRESH LUÔN CHẠY MỖI GIÂY
  if (now - lastOLEDUpdate >= 1000) {
    lastOLEDUpdate = now;
    updateOLED();
  }

  // 2. SENSOR POLLING CHẠY MỖI 2 GIÂY
  if (now - lastSensorRead >= 2000) {
    lastSensorRead = now;

    // Đọc cảm biến
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    currentLux = lightMeter.readLightLevel();

    int soil_raw = analogRead(PIN_SOIL_ADC);
    currentSoil = map(soil_raw, 4095, 0, 0, 100);
    if (currentSoil < 0)
      currentSoil = 0;
    if (currentSoil > 100)
      currentSoil = 100;

    bool dhtError = isnan(t) || isnan(h);
    currentDHTError = dhtError;

    if (!dhtError) {
      currentTemp = t;
      currentHum = h;
    }

    // 3. KIỂM TRA ĐIỀU KIỆN PUBLISH
    bool shouldPublish = false;

    // Điều kiện A: Heartbeat (5 phút)
    if (now - lastMQTTPublish >= 300000UL) {
      shouldPublish = true;
    }

    // Điều kiện B: Thay đổi trạng thái lỗi DHT
    if (dhtError != lastDHTError) {
      shouldPublish = true;
    }

    // Điều kiện C & D: Biến thiên nhiệt độ >= 0.5C hoặc độ ẩm >= 3%
    if (!dhtError && lastSentTemp != -999.0) {
      if (abs(currentTemp - lastSentTemp) >= 0.5)
        shouldPublish = true;
      if (abs(currentHum - lastSentHum) >= 3.0)
        shouldPublish = true;
    }

    // Gửi ngay lần đầu tiên có dữ liệu hợp lệ
    if (lastSentTemp == -999.0 && !dhtError) {
      shouldPublish = true;
    }

    // 4. THỰC THI PUBLISH NẾU ĐẠT ĐIỀU KIỆN
    if (shouldPublish) {
      JsonDocument doc;

      // Dùng JWT Token thay cho Verify Code cũ
      doc["token"] = jwtToken;

      JsonArray sensors = doc["sensors"].to<JsonArray>();

      JsonObject tempObj = sensors.add<JsonObject>();
      tempObj["key"] = "temperature";
      if (dhtError)
        tempObj["value"] = nullptr;
      else
        tempObj["value"] = currentTemp;

      JsonObject humObj = sensors.add<JsonObject>();
      humObj["key"] = "humidity";
      if (dhtError)
        humObj["value"] = nullptr;
      else
        humObj["value"] = currentHum;

      JsonObject soilObj = sensors.add<JsonObject>();
      soilObj["key"] = "soil_moisture";
      soilObj["value"] = currentSoil;

      JsonObject lightObj = sensors.add<JsonObject>();
      lightObj["key"] = "light";
      lightObj["value"] = currentLux;

      String payload;
      serializeJson(doc, payload);

      Serial.print("Event Triggered! Publish message: ");
      Serial.println(payload);

      if (mqttClient.connected()) {
        if (mqttClient.publish(telemetry_topic, payload.c_str())) {
          // Lưu lại lịch sử để tính toán cho lần sau
          lastMQTTPublish = now;
          lastDHTError = dhtError;
          if (!dhtError) {
            lastSentTemp = currentTemp;
            lastSentHum = currentHum;
          }
        }
      } else {
        // Dù MQTT đứt kết nối, ta vẫn cập nhật lại State để tránh bị kẹt trong
        // logic tính sai lệch khiến payload in ra Serial liên tục mỗi 2s.
        lastMQTTPublish = now;
        lastDHTError = dhtError;
        if (!dhtError) {
          lastSentTemp = currentTemp;
          lastSentHum = currentHum;
        }
      }
    }
  }
}
