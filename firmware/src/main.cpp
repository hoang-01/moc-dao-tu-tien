#include "secrets.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Wire.h>

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
double total_exp = 0.0;
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
unsigned long lastLocalExpAccumulate = 0;
bool bh1750_detected = false;
int animFrame = 0;

// OLED Layout Constants
#define PLANT_X 36
#define EXP_BAR_X 106
#define EXP_BAR_Y 28
#define EXP_BAR_W 8
#define EXP_BAR_H 32

// Breakthrough / Level-Up State
String last_known_rank = "";
bool triggerLevelUpAnim = false;
unsigned long levelUpStartTime = 0;

// Topics
char telemetry_topic[100];
char status_topic[100];
char response_topic[100];

// JWT Token
String jwtToken = "";

struct PlantThresholds {
  float tempMin = 0.0;
  float tempMax = 0.0;
  float humMin = 0.0;
  float humMax = 0.0;
  float soilMin = 0.0;
  float soilMax = 0.0;
  float lightMin = 0.0;
  float lightMax = 0.0;
  bool hasThresholds = false;
};
PlantThresholds currentThresholds;

String classifySensorQuality(float value, float idealMin, float idealMax) {
  float idealRange = idealMax - idealMin;
  if (idealRange <= 0)
    idealRange = 1.0;

  if (value >= idealMin && value <= idealMax) {
    return "EXCELLENT";
  }

  float deviation = 0.0;
  if (value < idealMin) {
    deviation = (idealMin - value) / idealRange;
  } else {
    deviation = (value - idealMax) / idealRange;
  }

  if (deviation <= 0.10)
    return "GOOD";
  else if (deviation <= 0.25)
    return "FAIR";
  else if (deviation <= 0.50)
    return "POOR";
  else
    return "DANGER";
}

String getLocalOverallQuality() {
  if (currentDHTError) {
    return "ERROR";
  }
  if (!currentThresholds.hasThresholds) {
    return "FAIR";
  }

  String qTemp = classifySensorQuality(currentTemp, currentThresholds.tempMin,
                                       currentThresholds.tempMax);
  String qHum = classifySensorQuality(currentHum, currentThresholds.humMin,
                                      currentThresholds.humMax);
  String qSoil = classifySensorQuality(currentSoil, currentThresholds.soilMin,
                                       currentThresholds.soilMax);

  int worstIdx = 0;
  String qualities[3] = {qTemp, qHum, qSoil};
  for (int i = 0; i < 3; i++) {
    int idx = 2; // Default FAIR
    if (qualities[i] == "EXCELLENT")
      idx = 0;
    else if (qualities[i] == "GOOD")
      idx = 1;
    else if (qualities[i] == "FAIR")
      idx = 2;
    else if (qualities[i] == "POOR")
      idx = 3;
    else if (qualities[i] == "DANGER")
      idx = 4;

    if (idx > worstIdx) {
      worstIdx = idx;
    }
  }

  if (worstIdx == 0)
    return "EXCELLENT";
  if (worstIdx == 1)
    return "GOOD";
  if (worstIdx == 2)
    return "FAIR";
  if (worstIdx == 3)
    return "POOR";
  return "DANGER";
}

String getLocalRank(double exp) {
  if (exp >= 30000.0)
    return "Do Kiep";
  if (exp >= 15000.0)
    return "Dai Thua";
  if (exp >= 8000.0)
    return "Hoa Than";
  if (exp >= 4000.0)
    return "Nguyen Anh";
  if (exp >= 1500.0)
    return "Kim Dan";
  if (exp >= 500.0)
    return "Truc Co";
  if (exp >= 100.0)
    return "Luyen Khi";
  return "Pham Moc";
}

int getRankIndex(String rank) {
  if (rank == "Do Kiep" || rank == "Độ Kiếp")
    return 7;
  if (rank == "Dai Thua" || rank == "Đại Thừa")
    return 6;
  if (rank == "Hoa Than" || rank == "Hóa Thần")
    return 5;
  if (rank == "Nguyen Anh" || rank == "Nguyên Anh")
    return 4;
  if (rank == "Kim Dan" || rank == "Kim Đan")
    return 3;
  if (rank == "Truc Co" || rank == "Trúc Cơ")
    return 2;
  if (rank == "Luyen Khi" || rank == "Luyện Khí")
    return 1;
  return 0; // Pham Moc
}

double getRankProgress(double exp, double &current_min, double &next_min) {
  if (exp >= 30000.0) {
    current_min = 30000.0;
    next_min = 30000.0;
    return 1.0;
  } else if (exp >= 15000.0) {
    current_min = 15000.0;
    next_min = 30000.0;
  } else if (exp >= 8000.0) {
    current_min = 8000.0;
    next_min = 15000.0;
  } else if (exp >= 4000.0) {
    current_min = 4000.0;
    next_min = 8000.0;
  } else if (exp >= 1500.0) {
    current_min = 1500.0;
    next_min = 4000.0;
  } else if (exp >= 500.0) {
    current_min = 500.0;
    next_min = 1500.0;
  } else if (exp >= 100.0) {
    current_min = 100.0;
    next_min = 500.0;
  } else {
    current_min = 0.0;
    next_min = 100.0;
  }
  return (exp - current_min) / (next_min - current_min);
}

bool isEnvironmentOptimal() {
  if (!currentThresholds.hasThresholds)
    return false;
  return getLocalOverallQuality() == "EXCELLENT";
}

void drawBootScreen(int frame) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setCursor(16, 5);
  display.println("MOC DAO TU TIEN");

  display.setCursor(30, 20);
  display.printf("Booting... %d%%", frame * 10);

  // Progress bar
  int width = (frame * 100) / 10;
  display.drawRect(14, 35, 100, 8, SSD1306_WHITE);
  display.fillRect(14, 35, width, 8, SSD1306_WHITE);

  // Sprout animation above progress bar
  int plantX = 14 + width;
  if (plantX > 109)
    plantX = 109;

  display.drawLine(plantX, 32, plantX, 26, SSD1306_WHITE);
  if ((frame / 2) % 2 == 0) {
    display.drawLine(plantX, 29, plantX - 3, 27, SSD1306_WHITE);
  } else {
    display.drawLine(plantX, 29, plantX + 3, 27, SSD1306_WHITE);
  }

  display.display();
}

void drawWifiConnectingScreen(int attempt, int frame) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(16, 5);
  display.println("=== WIFI CONNECT ===");

  display.setCursor(0, 20);
  display.printf("SSID: %s", WiFi.SSID().substring(0, 15).c_str());
  display.setCursor(0, 32);
  display.printf("Attempt %d/3...", attempt);

  // Animate WiFi symbol
  int x = 64;
  int y = 55;
  display.fillCircle(x, y, 2, SSD1306_WHITE);
  int step = frame % 4;
  if (step >= 1) {
    display.drawCircleHelper(x, y, 6, 1, SSD1306_WHITE);
    display.drawCircleHelper(x, y, 6, 2, SSD1306_WHITE);
  }
  if (step >= 2) {
    display.drawCircleHelper(x, y, 12, 1, SSD1306_WHITE);
    display.drawCircleHelper(x, y, 12, 2, SSD1306_WHITE);
  }
  if (step >= 3) {
    display.drawCircleHelper(x, y, 18, 1, SSD1306_WHITE);
    display.drawCircleHelper(x, y, 18, 2, SSD1306_WHITE);
  }

  display.display();
}

void drawWifiRetryScreen(int attempt, int secondsRemaining) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(16, 5);
  display.println("=== WIFI RETRY ===");

  display.setCursor(0, 20);
  display.printf("Attempt %d failed!", attempt);
  display.setCursor(0, 35);
  display.printf("Retrying in %d seconds", secondsRemaining);

  // Draw warning symbol
  display.drawTriangle(105, 55, 115, 35, 125, 55, SSD1306_WHITE);
  display.setCursor(113, 40);
  display.print("!");

  display.display();
}

void configModeCallback(WiFiManager *myWiFiManager) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== WIFI CONFIG ===");
  display.println("");
  display.printf("AP: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
  display.println("IP: 192.168.4.1");
  display.println("");
  display.println("Connect to ESP32 AP");
  display.println("to configure WiFi...");
  display.display();
}

void drawSproutAnimation(String quality, bool isOffline, int frame) {
  // Ground line
  display.drawFastHLine(PLANT_X - 16, 60, 32, SSD1306_WHITE);

  if (isOffline) {
    // Wilted plant + offline icon
    display.drawLine(PLANT_X, 60, PLANT_X - 3, 53, SSD1306_WHITE);
    display.drawLine(PLANT_X - 3, 53, PLANT_X - 9, 50, SSD1306_WHITE);
    display.drawLine(PLANT_X - 3, 53, PLANT_X - 10, 57, SSD1306_WHITE);
    display.drawLine(PLANT_X - 9, 50, PLANT_X - 14, 53, SSD1306_WHITE);

    // Flashing Offline WiFi symbol (slashed)
    if ((frame / 2) % 2 == 0) {
      int wx = 72;
      int wy = 45;
      display.fillCircle(wx, wy, 2, SSD1306_WHITE);
      display.drawCircleHelper(wx, wy, 6, 1, SSD1306_WHITE);
      display.drawCircleHelper(wx, wy, 6, 2, SSD1306_WHITE);
      display.drawCircleHelper(wx, wy, 12, 1, SSD1306_WHITE);
      display.drawCircleHelper(wx, wy, 12, 2, SSD1306_WHITE);
      display.drawLine(wx - 10, wy - 10, wx + 10, wy + 10, SSD1306_WHITE);
    }
    return;
  }

  if (quality == "ERROR") {
    // Wilted plant + warning sign
    display.drawLine(PLANT_X, 60, PLANT_X - 3, 53, SSD1306_WHITE);
    display.drawLine(PLANT_X - 3, 53, PLANT_X - 9, 50, SSD1306_WHITE);
    display.drawLine(PLANT_X - 3, 53, PLANT_X - 10, 57, SSD1306_WHITE);
    display.drawLine(PLANT_X - 9, 50, PLANT_X - 14, 53, SSD1306_WHITE);

    if ((frame / 2) % 2 == 0) {
      display.drawTriangle(67, 50, 77, 30, 87, 50, SSD1306_WHITE);
      display.setCursor(75, 35);
      display.print("!");
    }
    return;
  }

  if (quality == "EXCELLENT" || quality == "GOOD") {
    // Swaying happy sprout
    int swayOffsets[4] = {-1, 0, 1, 0};
    int leafOffsets[4] = {-1, 1, 1, -1};
    int sway = swayOffsets[frame % 4];
    int leafSway = leafOffsets[frame % 4];

    display.drawLine(PLANT_X, 60, PLANT_X, 52, SSD1306_WHITE);
    display.drawLine(PLANT_X, 52, PLANT_X + sway, 44, SSD1306_WHITE);
    display.drawLine(PLANT_X, 52, PLANT_X - 8, 48 - leafSway, SSD1306_WHITE);
    display.drawLine(PLANT_X + sway, 44, PLANT_X + 10 + sway, 40 + leafSway,
                     SSD1306_WHITE);

    // Rising particles of energy (Spiritual Qi)
    int numParticles = (quality == "EXCELLENT") ? 4 : 2;
    int speedFactor = (quality == "EXCELLENT") ? 4 : 2;
    for (int i = 0; i < numParticles; i++) {
      int p_y = 58 - (((frame * speedFactor + i * 12) % 36));
      int x_offsets[4] = {-8, 6, -3, 8};
      int p_x = PLANT_X + x_offsets[i % 4] + ((frame + i) % 3 - 1);
      if (p_y > 24 && p_y < 60) {
        display.drawPixel(p_x, p_y, SSD1306_WHITE);
      }
    }
  } else if (quality == "FAIR" || quality == "POOR") {
    // Normal/Static plant
    display.drawLine(PLANT_X, 60, PLANT_X, 52, SSD1306_WHITE);
    display.drawLine(PLANT_X, 52, PLANT_X, 45, SSD1306_WHITE);

    if (quality == "POOR") {
      display.drawLine(PLANT_X, 52, PLANT_X - 9, 54, SSD1306_WHITE);
      display.drawLine(PLANT_X, 45, PLANT_X + 9, 47, SSD1306_WHITE);

      // Falling "Tu khi" (dark particles) - slow/sparse (1 particle)
      int p_y = 44 + ((frame) % 16);
      int p_x = PLANT_X - 6 + ((frame / 4) % 2 == 0 ? 1 : -1);
      if (p_y > 44 && p_y < 60) {
        display.drawPixel(p_x, p_y, SSD1306_WHITE);
      }
    } else {
      display.drawLine(PLANT_X, 52, PLANT_X - 8, 50, SSD1306_WHITE);
      display.drawLine(PLANT_X, 45, PLANT_X + 8, 43, SSD1306_WHITE);
    }
  } else if (quality == "DANGER") {
    // Wilted plant
    display.drawLine(PLANT_X, 60, PLANT_X - 3, 53, SSD1306_WHITE);
    display.drawLine(PLANT_X - 3, 53, PLANT_X - 9, 50, SSD1306_WHITE);
    display.drawLine(PLANT_X - 3, 53, PLANT_X - 10, 57, SSD1306_WHITE);
    display.drawLine(PLANT_X - 9, 50, PLANT_X - 14, 53, SSD1306_WHITE);

    if ((frame / 2) % 2 == 0) {
      display.drawTriangle(67, 50, 77, 30, 87, 50, SSD1306_WHITE);
      display.setCursor(75, 35);
      display.print("!");
    }

    // Falling "Tu khi" (dark particles) - fast/dense (3 particles)
    for (int i = 0; i < 3; i++) {
      int p_y = 44 + (((frame * 3 + i * 6) % 16));
      int x_offsets[3] = {-9, 5, 0};
      int p_x = PLANT_X + x_offsets[i] + ((frame + i) % 2);
      if (p_y > 44 && p_y < 60) {
        display.drawPixel(p_x, p_y, SSD1306_WHITE);
      }
    }
  }
}

void drawExpBar(double exp) {
  double current_min = 0.0;
  double next_min = 0.0;
  double progress = getRankProgress(exp, current_min, next_min);

  // Draw EXP bar frame
  display.drawRect(EXP_BAR_X, EXP_BAR_Y, EXP_BAR_W, EXP_BAR_H, SSD1306_WHITE);

  // Fill progress (bottom-up) with 2px padding
  int inside_h = EXP_BAR_H - 4;
  int fill_h = (int)(progress * inside_h);
  if (fill_h > 0) {
    display.fillRect(EXP_BAR_X + 2, EXP_BAR_Y + EXP_BAR_H - 2 - fill_h,
                     EXP_BAR_W - 4, fill_h, SSD1306_WHITE);
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  bool isOffline = (WiFi.status() != WL_CONNECTED) || (!mqttClient.connected());
  String quality = getLocalOverallQuality();
  unsigned long now = millis();

  // Check breakthrough / level-up trigger
  String current_rank = getLocalRank(total_exp);
  if (last_known_rank != "" && last_known_rank != "Chưa rõ" &&
      last_known_rank != "Chua ro") {
    if (getRankIndex(current_rank) > getRankIndex(last_known_rank)) {
      triggerLevelUpAnim = true;
      levelUpStartTime = now;
    }
  }
  last_known_rank = current_rank;

  if (triggerLevelUpAnim) {
    if (now - levelUpStartTime > 2500) {
      triggerLevelUpAnim = false;
    }
  }

  // --- Row 1: Status & Delta ---
  display.setCursor(0, 0);
  if (triggerLevelUpAnim) {
    display.print("TT: DOT PHA!");
  } else if (isOffline) {
    display.print("TT: MAT KET NOI");
  } else if (quality == "ERROR") {
    display.print("TT: LOI CAM BIEN");
  } else {
    double delta = 0.0;
    if (quality == "EXCELLENT")
      delta = 1.0;
    else if (quality == "GOOD")
      delta = 0.5;
    else if (quality == "FAIR")
      delta = 0.0;
    else if (quality == "POOR")
      delta = -0.3;
    else if (quality == "DANGER")
      delta = -0.8;

    if (quality == "EXCELLENT") {
      display.printf("TT: EXCELLENT (%+.1f)", delta);
    } else if (quality == "GOOD") {
      display.printf("TT: GOOD (%+.1f)", delta);
    } else if (quality == "FAIR") {
      display.printf("TT: FAIR (%+.1f)", delta);
    } else if (quality == "POOR") {
      display.printf("TT: POOR (%+.1f)", delta);
    } else {
      display.printf("TT: DANGER (%+.1f)", delta);
    }
  }

  // --- Row 2: Rank & EXP ---
  display.setCursor(0, 12);
  String display_rank = rank_name;
  if (rank_name == "Luyện Khí")
    display_rank = "Luyen Khi";
  else if (rank_name == "Trúc Cơ")
    display_rank = "Truc Co";
  else if (rank_name == "Kim Đan")
    display_rank = "Kim Dan";
  else if (rank_name == "Nguyên Anh")
    display_rank = "Nguyen Anh";
  else if (rank_name == "Hóa Thần")
    display_rank = "Hoa Than";
  else if (rank_name == "Đại Thừa")
    display_rank = "Dai Thua";
  else if (rank_name == "Độ Kiếp")
    display_rank = "Do Kiep";
  else if (rank_name == "Phàm Mộc")
    display_rank = "Pham Moc";
  else if (rank_name == "Chưa rõ")
    display_rank = "Chua ro";

  display.printf("%s : %.1f", display_rank.c_str(), total_exp);

  // --- Plant Animation ---
  drawSproutAnimation(quality, isOffline, animFrame);

  if (triggerLevelUpAnim) {
    // Draw expanding rings of light centered around the plant center (36, 50)
    unsigned long elapsed = now - levelUpStartTime;
    int r1 = (elapsed / 10) % 35;
    int r2 = ((elapsed + 400) / 10) % 35;
    int r3 = ((elapsed + 800) / 10) % 35;

    display.drawCircle(PLANT_X, 50, r1, SSD1306_WHITE);
    if (elapsed >= 400) {
      display.drawCircle(PLANT_X, 50, r2, SSD1306_WHITE);
    }
    if (elapsed >= 800) {
      display.drawCircle(PLANT_X, 50, r3, SSD1306_WHITE);
    }

    // Flash DOT PHA! text on the right
    if ((elapsed / 200) % 2 == 0) {
      display.setCursor(94, 32);
      display.print("DOT");
      display.setCursor(94, 44);
      display.print("PHA!");
    }
  } else {
    // Draw vertical EXP bar
    drawExpBar(total_exp);
  }

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
        total_exp = doc["total_exp"].as<double>();
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

      if (doc.containsKey("next_reward_in_seconds")) {
        long next_sec = doc["next_reward_in_seconds"].as<long>();
        lastLocalExpAccumulate = millis() - (60 - next_sec) * 1000UL;
        Serial.printf("Next reward in: %ld seconds. Synced local timer.\n",
                      next_sec);
      }

      Serial.println("=== SERVER UPDATE ===");
      Serial.printf("EXP    : %.1f\n", total_exp);
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
      // Buộc gửi telemetry ngay lập tức để đồng bộ lại dữ liệu với server khi
      // kết nối lại
      lastMQTTPublish = 0;
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

  Serial.println("--- KHAO SAT BUS I2C0 (OLED) ---");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("  -> Tim thay thiet bi o dia chi: 0x%02X\n", address);
    }
  }
  Serial.println("--------------------------------");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    // Ép tốc độ I2C về chuẩn 100kHz để tránh nghẽn/xung đột với BH1750
    Wire.setClock(100000);

    for (int frame = 0; frame <= 10; frame++) {
      drawBootScreen(frame);
      delay(200);
    }
  }

  // Khởi tạo Cảm biến
  dht.begin();

  // Mở bus I2C riêng (Wire1) cho BH1750: SDA = 18, SCL = 19
  Wire1.begin(18, 19);

  Serial.println("--- KHAO SAT BUS I2C1 (BH1750) ---");
  for (byte address = 1; address < 127; address++) {
    Wire1.beginTransmission(address);
    byte error = Wire1.endTransmission();
    if (error == 0) {
      Serial.printf("  -> Tim thay thiet bi o dia chi: 0x%02X\n", address);
    }
  }
  Serial.println("----------------------------------");

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1)) {
    Serial.println(F("BH1750 initialized"));
    bh1750_detected = true;
  } else {
    Serial.println(F("Error initializing BH1750, check wiring/address!"));
    bh1750_detected = false;
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
  wm.setAPCallback(configModeCallback);

  String apName = "MocDao-" + String(DEFAULT_PLANT_CODE).substring(0, 6);
  bool connected = false;

  WiFi.mode(WIFI_STA);

  // Thử kết nối lại 3 lần nếu có credentials đã lưu
  if (WiFi.SSID() != "") {
    Serial.printf("Saved SSID found: %s. Attempting manual reconnect...\n",
                  WiFi.SSID().c_str());
    for (int attempt = 1; attempt <= 3; attempt++) {
      WiFi.begin();

      unsigned long startAttempt = millis();
      int animTick = 0;
      while (millis() - startAttempt < 15000) {
        if (WiFi.status() == WL_CONNECTED) {
          connected = true;
          break;
        }
        drawWifiConnectingScreen(attempt, animTick++);
        delay(300);
      }

      if (connected) {
        Serial.println("Manual WiFi reconnect successful!");
        break;
      } else {
        Serial.printf("Attempt %d failed.\n", attempt);
        for (int sec = 3; sec > 0; sec--) {
          drawWifiRetryScreen(attempt, sec);
          delay(1000);
        }
      }
    }
  }

  // Nếu không kết nối được thủ công hoặc chưa có credentials, mở Portal cấu
  // hình
  if (!connected) {
    Serial.println("Starting config portal...");
    if (wm.startConfigPortal(apName.c_str())) {
      connected = true;
      Serial.println("WiFi connected via Config Portal!");
    } else {
      Serial.println("Config Portal timeout or failed!");
    }
  }

  bool res = connected;

  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("WiFi connected!");

    // Thực hiện HTTP Auth lấy Token
    HTTPClient http;
    String authUrl =
        String(SERVER_HOST) + "/api/devices/" + DEFAULT_PLANT_CODE + "/auth";
    Serial.print("Authenticating with ");
    Serial.println(authUrl);

    http.begin(authUrl);
    http.addHeader("Content-Type", "application/json");

    String payload =
        "{\"verify_code\":\"" + String(DEFAULT_VERIFY_CODE) + "\"}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode == 200) {
      String response = http.getString();
      JsonDocument doc;
      deserializeJson(doc, response);
      jwtToken = doc["token"].as<String>();
      Serial.println("Auth Success! Token acquired.");

      if (doc.containsKey("total_exp")) {
        total_exp = doc["total_exp"].as<double>();
      }
      if (doc.containsKey("rank_name")) {
        rank_name = doc["rank_name"].as<String>();
      }

      if (doc.containsKey("next_reward_in_seconds")) {
        long next_sec = doc["next_reward_in_seconds"].as<long>();
        lastLocalExpAccumulate = millis() - (60 - next_sec) * 1000UL;
        Serial.printf("Next reward in: %ld seconds. Synced local timer.\n",
                      next_sec);
      }

      // Trích xuất các trường ngưỡng lý tưởng từ đối tượng thresholds
      if (doc.containsKey("thresholds") && !doc["thresholds"].isNull()) {
        currentThresholds.tempMin =
            doc["thresholds"]["temperature_min"].as<float>();
        currentThresholds.tempMax =
            doc["thresholds"]["temperature_max"].as<float>();
        currentThresholds.humMin =
            doc["thresholds"]["humidity_min"].as<float>();
        currentThresholds.humMax =
            doc["thresholds"]["humidity_max"].as<float>();
        currentThresholds.soilMin =
            doc["thresholds"]["soil_moisture_min"].as<float>();
        currentThresholds.soilMax =
            doc["thresholds"]["soil_moisture_max"].as<float>();
        currentThresholds.lightMin = doc["thresholds"]["light_min"].as<float>();
        currentThresholds.lightMax = doc["thresholds"]["light_max"].as<float>();
        currentThresholds.hasThresholds = true;

        Serial.println("=== Ngưỡng lý tưởng đã đồng bộ ===");
        Serial.printf("  Nhiệt độ : %.1f - %.1f C\n", currentThresholds.tempMin,
                      currentThresholds.tempMax);
        Serial.printf("  Độ ẩm KK : %.1f - %.1f %%\n", currentThresholds.humMin,
                      currentThresholds.humMax);
        Serial.printf("  Độ ẩm đất: %.1f - %.1f %%\n",
                      currentThresholds.soilMin, currentThresholds.soilMax);
        Serial.printf("  Ánh sáng : %.1f - %.1f lux\n",
                      currentThresholds.lightMin, currentThresholds.lightMax);
      } else {
        Serial.println("Không tìm thấy cấu hình ngưỡng trong phản hồi Auth.");
      }
    } else {
      Serial.printf("Auth Failed! Code: %d\n", httpResponseCode);
      Serial.println(http.getString());
    }
    http.end();
  }

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  // Mở rộng Buffer Size để chứa vừa chuỗi JWT Token dài
  mqttClient.setBufferSize(1024);

  // Khởi tạo để gửi bản tin ngay sau khi boot
  lastMQTTPublish = millis() - 300000UL;
  lastLocalExpAccumulate = millis();
}

// Bộ lọc nhiễu phần mềm (Median Filter + Outlier Rejection)
int readFilteredSoilMoisture() {
  int samples[15];
  for (int i = 0; i < 15; i++) {
    samples[i] = analogRead(PIN_SOIL_ADC);
    delay(10);
  }
  // Sắp xếp mảng tăng dần (Bubble sort đơn giản)
  for (int i = 0; i < 14; i++) {
    for (int j = i + 1; j < 15; j++) {
      if (samples[i] > samples[j]) {
        int temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }
  // Loại bỏ 5 phần tử đầu và 5 phần tử cuối, lấy trung bình 5 phần tử ở giữa
  long sum = 0;
  for (int i = 5; i < 10; i++) {
    sum += samples[i];
  }
  return sum / 5;
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
  if (now - lastOLEDUpdate >= 200) {
    lastOLEDUpdate = now;
    animFrame = (animFrame + 1) % 100;
    updateOLED();
  }

  // 3. TỰ ĐỘNG TÍCH LŨY ĐIỂM CỤC BỘ MỖI 6 GIÂY (MỊN)
  if (now - lastLocalExpAccumulate >= 6000UL) {
    lastLocalExpAccumulate = now;

    // Đóng băng nếu mất kết nối hoặc lỗi cảm biến
    bool isOffline =
        (WiFi.status() != WL_CONNECTED) || (!mqttClient.connected());
    String quality = getLocalOverallQuality();

    if (!isOffline && quality != "ERROR") {
      double delta = 0.0;
      if (quality == "EXCELLENT")
        delta = 1.0;
      else if (quality == "GOOD")
        delta = 0.5;
      else if (quality == "FAIR")
        delta = 0.0;
      else if (quality == "POOR")
        delta = -0.3;
      else if (quality == "DANGER")
        delta = -0.8;

      double old_exp = total_exp;
      total_exp = total_exp + delta;
      if (total_exp < 0.0) {
        total_exp = 0.0;
      }

      rank_name = getLocalRank(total_exp);
      if (delta != 0.0) {
        Serial.printf("[Local EXP Update] Quality: %s | Delta: %+.1f | EXP: "
                      "%.1f -> %.1f | Rank: %s\n",
                      quality.c_str(), delta, old_exp, total_exp,
                      rank_name.c_str());
        updateOLED();
      }
    } else {
      if (isOffline) {
        Serial.println("[Local EXP] Frozen: Device is offline.");
      } else {
        Serial.println("[Local EXP] Frozen: Sensor error.");
      }
    }
  }

  // 2. SENSOR POLLING CHẠY MỖI 2 GIÂY
  if (now - lastSensorRead >= 2000) {
    lastSensorRead = now;

    // Đọc cảm biến
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (bh1750_detected) {
      currentLux = lightMeter.readLightLevel();
    } else {
      currentLux = -2.0;
    }

    // Đọc cảm biến độ ẩm đất bằng bộ lọc nhiễu phần mềm (Median Filter)
    int soil_raw = readFilteredSoilMoisture();

    // In giá trị raw ra Serial để user dễ dàng căn chỉnh (Calibrate) lại các
    // mốc 4095 và 0
    Serial.printf("Soil Raw: %d\n", soil_raw);

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

    // In thông số cảm biến liên tục mỗi 2 giây để tiện theo dõi và chọn ngưỡng
    Serial.printf("[Sensor Log] Temp: %.1f C | Hum: %.1f%% | Soil Raw: %d "
                  "(Mapped: %d%%) | Light: %.1f lux | DHT Error: %s\n",
                  currentTemp, currentHum, soil_raw, currentSoil, currentLux,
                  dhtError ? "ERROR" : "OK");

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
        tempObj["value"] = -999.0;
      else
        tempObj["value"] = currentTemp;

      JsonObject humObj = sensors.add<JsonObject>();
      humObj["key"] = "humidity";
      if (dhtError)
        humObj["value"] = -999.0;
      else
        humObj["value"] = currentHum;

      JsonObject soilObj = sensors.add<JsonObject>();
      soilObj["key"] = "soil_moisture";
      soilObj["value"] = currentSoil;

      JsonObject lightObj = sensors.add<JsonObject>();
      lightObj["key"] = "light";
      if (currentLux < 0.0)
        lightObj["value"] = -999.0;
      else
        lightObj["value"] = currentLux;

      String payload;
      serializeJson(doc, payload);

      Serial.print("Event Triggered! Publish message: ");
      Serial.println(payload);

      if (mqttClient.connected()) {
        bool success = mqttClient.publish(telemetry_topic, payload.c_str());

        // Luôn cập nhật thời gian để tránh kẹt vòng lặp gửi liên tục mỗi 2s
        // trong trường hợp publish thất bại
        lastMQTTPublish = now;

        if (success) {
          // Chỉ lưu lịch sử nhiệt độ/độ ẩm mới khi đã gửi thành công lên server
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
