#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Pins (I2C)
#define SDA_PIN 21
#define SCL_PIN 22
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Wi-Fi & Server Details
const char* ssid = "Hwjunction";
const char* password = "forged@forge";
const char* serverUrl = "http://192.168.55.191:8000/api/device/status/";

String serverStatus = "Offline";
unsigned long lastPollTime = 0;
const unsigned long pollInterval = 5000; // 5 seconds
// TCS3200 Pins
#define S0 17
#define S1 5
#define S2 18
#define S3 19
#define OUT 23
#define LED 16

void setup() {
  Serial.begin(115200);
  
  // Initialize Pins
  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(OUT, INPUT);

  // Frequency Scaling 20%
  digitalWrite(S0, HIGH); digitalWrite(S1, LOW);
  digitalWrite(LED, HIGH); // Turn on LED

  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 allocation failed");
    // Try alternate address
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      for(;;);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting Wi-Fi...");
  display.display();

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected, IP:");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Wi-Fi Connected!");
  display.print("IP: "); display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

void loop() {
  // Read RED
  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  int r = pulseIn(OUT, LOW);
  
  // Read GREEN
  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  int g = pulseIn(OUT, LOW);
  
  // Read BLUE
  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  int b = pulseIn(OUT, LOW);

  // Poll Server
  if (millis() - lastPollTime >= pollInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        String payload = http.getString();
        serverStatus = (httpResponseCode == 200) ? "Connected" : String("Err: ") + String(httpResponseCode);
      } else {
        serverStatus = "Offline";
      }
      http.end();
    } else {
      serverStatus = "No Wi-Fi";
    }
    lastPollTime = millis();
  }

  // Update Display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("--- COLOR SCAN ---");
  display.setCursor(0, 15);
  display.print("R: "); display.println(r);
  display.print("G: "); display.println(g);
  display.print("B: "); display.println(b);
  display.setCursor(0, 45);
  display.print("Srv: ");
  display.println(serverStatus);
  display.display();

  // Print to Serial for backup
  Serial.print("R:"); Serial.print(r);
  Serial.print(" G:"); Serial.print(g);
  Serial.print(" B:"); Serial.println(b);

  delay(500);
}