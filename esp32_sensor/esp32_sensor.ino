/*
 * ============================================================
 *  ColorPro — ESP32 Connection Test
 *  Wi-Fi  +  Backend Ping  +  OLED Display
 *  (No sensor — connection testing only)
 *
 *  Required Libraries (Arduino Library Manager):
 *    - Adafruit SSD1306
 *    - Adafruit GFX
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================== CHANGE THESE =========================
const char* WIFI_SSID     = "protosem";
const char* WIFI_PASSWORD = "proto123";
const char* SERVER_IP     = "192.168.56.23";  // Your PC's LAN IP
const int   SERVER_PORT   = 8000;
// ============================================================

// ─── OLED (128x64, I2C) ─────────────────────────────────────
#define SDA_PIN   21
#define SCL_PIN   22
#define OLED_ADDR 0x3C
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ─── State ───────────────────────────────────────────────────
bool   serverOnline  = false;
int    pingCode      = 0;
String localIP       = "---.---.---.---";
unsigned long lastPing = 0;
const  unsigned long PING_EVERY = 10000; // 10 seconds

// ─── Build API URL ───────────────────────────────────────────
String apiUrl(const char* path) {
  return "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + path;
}

// ─── OLED: draw the status screen ────────────────────────────
void updateDisplay() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // ── Title bar ──
  oled.setTextSize(1);
  oled.setCursor(22, 0);
  oled.print("ColorPro ESP32");
  oled.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // ── Wi-Fi row ──
  oled.setCursor(0, 13);
  oled.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    oled.println("Connected");
    oled.setCursor(0, 24);
    oled.print("IP: ");
    oled.println(localIP);
  } else {
    oled.println("OFFLINE");
    oled.setCursor(0, 24);
    oled.println("Reconnecting...");
  }

  // ── Server row ──
  oled.drawLine(0, 35, 127, 35, SSD1306_WHITE);
  oled.setCursor(0, 38);
  oled.print("Server: ");
  oled.println(serverOnline ? "ONLINE" : "OFFLINE");

  oled.setCursor(0, 49);
  oled.print("Last ping: ");
  oled.print(pingCode > 0 ? String(pingCode) : "---");

  // ── Footer ──
  oled.drawLine(0, 58, 127, 58, SSD1306_WHITE);
  oled.setCursor(10, 59);
  oled.print(SERVER_IP);
  oled.print(":");
  oled.print(SERVER_PORT);

  oled.display();
}

// ─── Connect to Wi-Fi ────────────────────────────────────────
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("[WiFi] Connecting to %s ...\n", WIFI_SSID);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Connecting to WiFi");
  oled.println(WIFI_SSID);
  oled.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
    // Animate dots on OLED
    oled.print(".");
    oled.display();
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    localIP = WiFi.localIP().toString();
    Serial.printf("[WiFi] Connected! IP: %s\n", localIP.c_str());
  } else {
    Serial.println("[WiFi] Failed to connect");
    localIP = "N/A";
  }
}

// ─── Ping the backend ────────────────────────────────────────
void pingServer() {
  if (WiFi.status() != WL_CONNECTED) {
    serverOnline = false;
    pingCode = 0;
    return;
  }

  HTTPClient http;
  http.begin(apiUrl("/api/device/ping/"));
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  pingCode = http.POST("{}");

  if (pingCode == 200) {
    serverOnline = true;
    Serial.println("[Ping] Server: ONLINE (200)");
  } else if (pingCode > 0) {
    serverOnline = false;
    Serial.printf("[Ping] Unexpected code: %d\n", pingCode);
  } else {
    serverOnline = false;
    Serial.printf("[Ping] Failed: %s\n", http.errorToString(pingCode).c_str());
  }

  http.end();
}

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== ColorPro ESP32 Connection Test =====");

  // Init I2C and OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // Try alternate address 0x3D
    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("[OLED] Init FAILED — check wiring");
      for (;;);  // Stop here so you can debug
    }
  }
  oled.clearDisplay();
  oled.display();

  // Splash
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 8);
  oled.println("ColorPro");
  oled.setTextSize(1);
  oled.setCursor(28, 30);
  oled.println("Shade Tester");
  oled.setCursor(28, 45);
  oled.println("Starting up...");
  oled.display();
  delay(1500);

  // Connect WiFi
  connectWiFi();

  // First ping
  pingServer();
  lastPing = millis();

  // Draw initial screen
  updateDisplay();
}

// ═════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Ping server every PING_EVERY ms
  if (now - lastPing >= PING_EVERY) {
    pingServer();
    lastPing = now;
  }

  // Refresh OLED
  updateDisplay();

  delay(500);
}