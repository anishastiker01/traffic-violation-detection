/*
 * Traffic Signal Violation Detection System
 * ESP32-CAM Module
 *
 * Logic:
 * - IR sensor detects vehicle at red light
 * - 5-second grace period given (for vehicles that cannot stop in time)
 * - If vehicle still detected after 5s → capture photo → send to server
 *
 * Hardware:
 * - ESP32-CAM (AI-Thinker)
 * - IR proximity sensor (GPIO pin 13)
 * - Traffic light signal wire (GPIO pin 14) [HIGH = RED, LOW = GREEN]
 * - LED flash (GPIO pin 4, built-in)
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"

// ─── WiFi Credentials ────────────────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ─── Server Config ────────────────────────────────────────────────────────────
const char* SERVER_URL    = "http://YOUR_SERVER_IP:5000/upload";
const char* LOCATION_ID   = "JUNCTION_01";   // Change per camera location

// ─── Pin Config ───────────────────────────────────────────────────────────────
#define IR_SENSOR_PIN     13   // IR sensor output
#define SIGNAL_PIN        14   // Traffic light signal (HIGH = RED)
#define FLASH_LED_PIN      4   // Built-in flash LED

// ─── Timing Config ────────────────────────────────────────────────────────────
#define GRACE_PERIOD_MS   5000  // 5 second grace period
#define COOLDOWN_MS       10000 // 10s cooldown between captures (same vehicle)

// ─── Camera Pin Map (AI-Thinker ESP32-CAM) ───────────────────────────────────
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ─── NTP Time ─────────────────────────────────────────────────────────────────
const char* NTP_SERVER    = "pool.ntp.org";
const long  GMT_OFFSET    = 19800;  // IST = UTC+5:30 = 19800 seconds
const int   DAYLIGHT_OFFSET = 0;

// ─── State Variables ──────────────────────────────────────────────────────────
bool     violationTimerStarted = false;
unsigned long violationStartTime = 0;
unsigned long lastCapturetime    = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(SIGNAL_PIN, INPUT);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  initCamera();
  connectWiFi();
  configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);

  Serial.println("[SYSTEM] Traffic Violation Detector Ready.");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  bool isRedLight    = digitalRead(SIGNAL_PIN) == HIGH;
  bool vehiclePresent = digitalRead(IR_SENSOR_PIN) == LOW; // LOW = object detected
  unsigned long now  = millis();

  if (isRedLight && vehiclePresent) {
    if (!violationTimerStarted) {
      // Start grace period
      violationTimerStarted = true;
      violationStartTime    = now;
      Serial.println("[ALERT] Vehicle detected at RED. Grace period started...");
    } else {
      unsigned long elapsed = now - violationStartTime;

      if (elapsed >= GRACE_PERIOD_MS) {
        // Violation confirmed — check cooldown
        if (now - lastCapturetime > COOLDOWN_MS) {
          Serial.println("[VIOLATION] Confirmed! Capturing photo...");
          captureAndSend();
          lastCapturetime = now;
        }
      } else {
        Serial.printf("[GRACE] %lu ms remaining...\n", GRACE_PERIOD_MS - elapsed);
      }
    }
  } else {
    // Reset if signal goes green or vehicle leaves
    if (violationTimerStarted) {
      Serial.println("[CLEAR] Vehicle cleared or signal changed. Resetting.");
    }
    violationTimerStarted = false;
  }

  delay(200);
}

// ─────────────────────────────────────────────────────────────────────────────
void captureAndSend() {
  // Flash on
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(100);

  camera_fb_t* fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    Serial.println("[ERROR] Camera capture failed.");
    return;
  }

  Serial.printf("[PHOTO] Captured %d bytes. Sending to server...\n", fb->len);

  // Get timestamp
  struct tm timeinfo;
  char timestamp[30];
  if (getLocalTime(&timeinfo)) {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  } else {
    snprintf(timestamp, sizeof(timestamp), "unknown_time");
  }

  // Send to server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Location-ID", LOCATION_ID);
    http.addHeader("X-Timestamp", timestamp);

    int responseCode = http.POST(fb->buf, fb->len);
    Serial.printf("[SERVER] Response code: %d\n", responseCode);
    http.end();
  } else {
    Serial.println("[ERROR] WiFi disconnected. Could not send.");
    connectWiFi();
  }

  esp_camera_fb_return(fb);
}

// ─────────────────────────────────────────────────────────────────────────────
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] Camera init failed: 0x%x\n", err);
    return;
  }
  Serial.println("[CAMERA] Initialized.");
}

// ─────────────────────────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Failed to connect.");
  }
}
