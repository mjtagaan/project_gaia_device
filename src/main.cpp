#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h"

// ================= BYPASS MODE =================
// Set to true to skip sensors and read from Firebase only (OLED test mode)
// Set to false for normal operation with sensors
#define BYPASS_SENSORS false
// ================================================

// ================= 1. USER CONFIGURATION =================
// WIFI SETTINGS
#define WIFI_SSID "<Your WiFi SSID>"
#define WIFI_PASSWORD "<Your WiFi Password>"

// FIREBASE SETTINGS
#define API_KEY "<Your Firebase API Key>"
#define DATABASE_URL "<Your Firebase Database URL>"

// SENSOR PINS
#define DHTPIN 4        // Digital Pin for Air (Safe for WiFi)
#define MOISTURE_PIN 34 // Analog Pin for Soil (ADC1 - Safe for WiFi)
#define DHTTYPE DHT22   // Sensor Type

// I2C PINS
#define SDA_PIN 21
#define SCL_PIN 22

// OLED SETTINGS
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// CALIBRATION (Adjust these after testing!)
const int DRY_VAL = 3500; // Value in air
const int WET_VAL = 1200; // Value in water

// ================= 2. GLOBAL OBJECTS =================
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// ================= 2.1 DISPLAY LOGIC =================
void drawStatusBar(int batteryPercent) {
  // 1. Divider Line
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // 2. WiFi Icon (Left)
  if (WiFi.status() == WL_CONNECTED) {
    display.drawBitmap(0, 0, wifi_connected_bits, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
  } else {
    display.drawBitmap(0, 0, wifi_disconnected_bits, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
  }

  // 3. Battery Icon (Right)
  // Battery Outline: 110, 1 -> 16x8
  display.drawRect(110, 1, 16, 8, SSD1306_WHITE); 
  // Positive Terminal (Bump)
  display.drawLine(126, 3, 126, 6, SSD1306_WHITE);
  
  // Fill Bar
  if (batteryPercent > 0) {
    int barWidth = map(constrain(batteryPercent, 0, 100), 0, 100, 0, 12);
    display.fillRect(112, 3, barWidth, 4, SSD1306_WHITE);
  }
}

// ================= FACE DRAWING (Primitives) =================
// Draws faces using Adafruit_GFX shapes - no bitmaps needed!
// Face area: y=12..63 (52px tall), x=0..127 (128px wide)
// Eye line Y≈28, Mouth Y≈50, Left eye X=44, Right eye X=84
void drawFace(int faceType) {
  switch (faceType) {

    case FACE_HAPPY: {
      // Big round filled eyes
      display.fillCircle(44, 28, 6, SSD1306_WHITE);
      display.fillCircle(84, 28, 6, SSD1306_WHITE);
      // U-shaped smile
      display.drawLine(38, 48, 44, 54, SSD1306_WHITE);
      display.drawLine(44, 54, 84, 54, SSD1306_WHITE);
      display.drawLine(84, 54, 90, 48, SSD1306_WHITE);
      display.drawLine(38, 49, 44, 55, SSD1306_WHITE);
      display.drawLine(44, 55, 84, 55, SSD1306_WHITE);
      display.drawLine(84, 55, 90, 49, SSD1306_WHITE);
      break;
    }

    case FACE_THIRSTY: {
      // X_X dead eyes (crossed lines, thick)
      // Left X
      display.drawLine(36, 20, 52, 36, SSD1306_WHITE);
      display.drawLine(52, 20, 36, 36, SSD1306_WHITE);
      display.drawLine(37, 20, 53, 36, SSD1306_WHITE);
      display.drawLine(53, 20, 37, 36, SSD1306_WHITE);
      display.drawLine(38, 20, 54, 36, SSD1306_WHITE);
      display.drawLine(54, 20, 38, 36, SSD1306_WHITE);
      // Right X
      display.drawLine(76, 20, 92, 36, SSD1306_WHITE);
      display.drawLine(92, 20, 76, 36, SSD1306_WHITE);
      display.drawLine(77, 20, 93, 36, SSD1306_WHITE);
      display.drawLine(93, 20, 77, 36, SSD1306_WHITE);
      display.drawLine(78, 20, 94, 36, SSD1306_WHITE);
      display.drawLine(94, 20, 78, 36, SSD1306_WHITE);
      // Flat line mouth
      display.fillRect(44, 50, 40, 3, SSD1306_WHITE);
      // Teardrop falling from right eye
      display.fillTriangle(104, 20, 101, 30, 107, 30, SSD1306_WHITE);
      display.fillCircle(104, 32, 4, SSD1306_WHITE);
      break;
    }

    case FACE_OVERWATERED: {
      // @_@ dizzy spiral eyes (concentric circles + center dot)
      display.drawCircle(44, 28, 10, SSD1306_WHITE);
      display.drawCircle(44, 28, 6, SSD1306_WHITE);
      display.fillCircle(44, 28, 2, SSD1306_WHITE);
      display.drawCircle(84, 28, 10, SSD1306_WHITE);
      display.drawCircle(84, 28, 6, SSD1306_WHITE);
      display.fillCircle(84, 28, 2, SSD1306_WHITE);
      // Wavy/queasy mouth (zigzag sine)
      display.drawLine(36, 50, 44, 46, SSD1306_WHITE);
      display.drawLine(44, 46, 52, 54, SSD1306_WHITE);
      display.drawLine(52, 54, 60, 46, SSD1306_WHITE);
      display.drawLine(60, 46, 68, 54, SSD1306_WHITE);
      display.drawLine(68, 54, 76, 46, SSD1306_WHITE);
      display.drawLine(76, 46, 84, 50, SSD1306_WHITE);
      // Sweat drops on sides
      display.fillCircle(20, 35, 2, SSD1306_WHITE);
      display.fillCircle(108, 35, 2, SSD1306_WHITE);
      break;
    }

    case FACE_HOT: {
      // >_< angry - V-shaped eyebrows slanting inward
      display.drawLine(30, 17, 56, 23, SSD1306_WHITE);
      display.drawLine(30, 18, 56, 24, SSD1306_WHITE);
      display.drawLine(30, 19, 56, 25, SSD1306_WHITE);
      display.drawLine(98, 17, 72, 23, SSD1306_WHITE);
      display.drawLine(98, 18, 72, 24, SSD1306_WHITE);
      display.drawLine(98, 19, 72, 25, SSD1306_WHITE);
      // Small angry dot eyes under brows
      display.fillCircle(44, 31, 4, SSD1306_WHITE);
      display.fillCircle(84, 31, 4, SSD1306_WHITE);
      // Angry frown (inverted U)
      display.drawLine(40, 56, 48, 48, SSD1306_WHITE);
      display.drawLine(48, 48, 80, 48, SSD1306_WHITE);
      display.drawLine(80, 48, 88, 56, SSD1306_WHITE);
      display.drawLine(40, 57, 48, 49, SSD1306_WHITE);
      display.drawLine(48, 49, 80, 49, SSD1306_WHITE);
      display.drawLine(80, 49, 88, 57, SSD1306_WHITE);
      // Heat waves above
      display.drawLine(20, 14, 24, 12, SSD1306_WHITE);
      display.drawLine(24, 12, 28, 14, SSD1306_WHITE);
      display.drawLine(100, 14, 104, 12, SSD1306_WHITE);
      display.drawLine(104, 12, 108, 14, SSD1306_WHITE);
      break;
    }

    case FACE_COLD: {
      // O_O wide shocked eyes with pupils
      display.drawCircle(44, 28, 10, SSD1306_WHITE);
      display.drawCircle(44, 28, 11, SSD1306_WHITE);
      display.fillCircle(44, 28, 4, SSD1306_WHITE);
      display.drawCircle(84, 28, 10, SSD1306_WHITE);
      display.drawCircle(84, 28, 11, SSD1306_WHITE);
      display.fillCircle(84, 28, 4, SSD1306_WHITE);
      // Zigzag chattering teeth mouth
      display.drawLine(38, 50, 46, 44, SSD1306_WHITE);
      display.drawLine(46, 44, 54, 50, SSD1306_WHITE);
      display.drawLine(54, 50, 62, 44, SSD1306_WHITE);
      display.drawLine(62, 44, 70, 50, SSD1306_WHITE);
      display.drawLine(70, 50, 78, 44, SSD1306_WHITE);
      display.drawLine(78, 44, 86, 50, SSD1306_WHITE);
      display.drawLine(38, 51, 46, 45, SSD1306_WHITE);
      display.drawLine(46, 45, 54, 51, SSD1306_WHITE);
      display.drawLine(54, 51, 62, 45, SSD1306_WHITE);
      display.drawLine(62, 45, 70, 51, SSD1306_WHITE);
      display.drawLine(70, 51, 78, 45, SSD1306_WHITE);
      display.drawLine(78, 45, 86, 51, SSD1306_WHITE);
      // Shiver lines on sides
      display.drawLine(8, 30, 16, 26, SSD1306_WHITE);
      display.drawLine(16, 26, 8, 22, SSD1306_WHITE);
      display.drawLine(120, 30, 112, 26, SSD1306_WHITE);
      display.drawLine(112, 26, 120, 22, SSD1306_WHITE);
      break;
    }

    case FACE_DARK: {
      // -_- sleeping closed eyes (thick horizontal bars)
      display.fillRect(34, 26, 20, 4, SSD1306_WHITE);
      display.fillRect(74, 26, 20, 4, SSD1306_WHITE);
      // Small peaceful mouth
      display.fillRect(56, 50, 16, 2, SSD1306_WHITE);
      // Zzz floating to the upper right
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(102, 13);
      display.print("Z");
      display.setTextSize(1);
      display.setCursor(110, 16);
      display.print("z");
      display.setCursor(116, 22);
      display.print("z");
      break;
    }

    case FACE_BRIGHT: {
      // Squinting ≡_≡ eyes (three lines per eye)
      // Left eye
      display.fillRect(34, 22, 20, 2, SSD1306_WHITE);
      display.fillRect(34, 27, 20, 2, SSD1306_WHITE);
      display.fillRect(34, 32, 20, 2, SSD1306_WHITE);
      // Right eye
      display.fillRect(74, 22, 20, 2, SSD1306_WHITE);
      display.fillRect(74, 27, 20, 2, SSD1306_WHITE);
      display.fillRect(74, 32, 20, 2, SSD1306_WHITE);
      // Wide cheery smile
      display.drawLine(34, 48, 40, 56, SSD1306_WHITE);
      display.drawLine(40, 56, 88, 56, SSD1306_WHITE);
      display.drawLine(88, 56, 94, 48, SSD1306_WHITE);
      display.drawLine(34, 49, 40, 57, SSD1306_WHITE);
      display.drawLine(40, 57, 88, 57, SSD1306_WHITE);
      display.drawLine(88, 57, 94, 49, SSD1306_WHITE);
      // Sun rays at top corners
      display.drawLine(6, 14, 14, 14, SSD1306_WHITE);
      display.drawLine(10, 12, 10, 16, SSD1306_WHITE);
      display.drawLine(114, 14, 122, 14, SSD1306_WHITE);
      display.drawLine(118, 12, 118, 16, SSD1306_WHITE);
      break;
    }
  }
}

void updateScreen(float temp, int moisture, float lux) {
  display.clearDisplay();
  
  // Update Battery Placeholder
  static int batteryPercent = 85; 
  
  // Draw Status Bar
  drawStatusBar(batteryPercent);

  // Determine Face Priority
  int faceType = FACE_HAPPY; // Default

  // 1. CRITICAL: Moisture (Thirsty < 30% | Overwatered > 85%)
  if (moisture < 30) {
    faceType = FACE_THIRSTY;
  } 
  else if (moisture > 85) {
    faceType = FACE_OVERWATERED;
  }
  // 2. WARNING: Temperature (Hot > 30C | Cold < 15C)
  else if (temp > 30.0) {
    faceType = FACE_HOT;
  }
  else if (temp < 15.0) {
    faceType = FACE_COLD;
  }
  // 3. MINOR: Light (Dark < 100 | Bright > 2000)
  else if (lux < 100) {
    faceType = FACE_DARK;
  }
  else if (lux > 2000) {
    faceType = FACE_BRIGHT;
  }
  // Else stays Happy

  // Draw face using primitives (guaranteed pixel-perfect rendering)
  drawFace(faceType);
  
  display.display();
}

// ================= 3. SETUP =================
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n========== GAIA SYSTEM STARTUP ==========");
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("I2C initialized on GPIO21 (SDA) and GPIO22 (SCL)");
  
  // Scan I2C bus
  Serial.println("\nScanning I2C bus...");
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(i, HEX);
      count++;
    }
  }
  Serial.print("Found ");
  Serial.print(count);
  Serial.println(" device(s)\n");
  
  // Initialize OLED - try 0x3C first, then 0x3D
  bool oledOK = false;
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✓ OLED initialized at 0x3C");
    oledOK = true;
  } else if(display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println("✓ OLED initialized at 0x3D");
    oledOK = true;
  } else {
    Serial.println("✗ OLED initialization FAILED!");
    Serial.println("  Check: VCC->3.3V, GND->GND, SDA->21, SCL->22");
  }
  
  if (oledOK) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 32);
    display.println("=== GAIA System ===");
    display.println();
    display.println("Initializing...");
    display.println("WiFi connecting...");
    display.display();
  }
  
#if !BYPASS_SENSORS
  // Initialize BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("✓ BH1750 initialized (ADDR->GND)");
  } else {
    Serial.println("✗ BH1750 initialization failed!");
    Serial.println("  Check: ADDR->GND for 0x23 address");
  }
  
  // Start Other Sensors
  dht.begin();
  pinMode(MOISTURE_PIN, INPUT);
  Serial.println("✓ DHT22 and Moisture sensors initialized");
#else
  Serial.println(">>> BYPASS MODE: Sensors disabled, reading from Firebase <<<");
#endif
  Serial.println();

  // Start Wi-Fi
  Serial.println("WiFi Configuration:");
  Serial.print("  SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("  Password Length: ");
  Serial.println(strlen(WIFI_PASSWORD));
  Serial.println();
  
  // Power cycle WiFi for clean start
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(500);
  WiFi.disconnect(true);
  delay(1000);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 50) {
    Serial.print(".");
    
    // Update OLED every 5 attempts
    if (wifiAttempts % 5 == 0) {
      display.clearDisplay();
      display.setCursor(0, 32);
      display.println("WiFi Connecting...");
      display.println();
      display.print("Attempt: ");
      display.print(wifiAttempts + 1);
      display.print("/50");
      for (int i = 0; i < (wifiAttempts / 5) % 4; i++) {
        display.print(".");
      }
      display.display();
    }
    
    // Retry connection every 10 attempts
    if (wifiAttempts > 0 && wifiAttempts % 10 == 0) {
      Serial.println();
      Serial.println("  Retrying connection...");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    
    delay(500);
    wifiAttempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✓ Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    display.clearDisplay();
    display.setCursor(0, 32);
    display.println("WiFi: Connected!");
    display.println();
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(3000);
  } else {
    Serial.println("✗ WiFi connection failed!");
    Serial.println();
    Serial.println("Troubleshooting:");
    Serial.println("  1. Check SSID name is correct");
    Serial.println("  2. Check password is correct");
    Serial.println("  3. Make sure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)");
    Serial.println("  4. Check WiFi router is powered on");
    Serial.println("  5. Move ESP32 closer to router");
    Serial.println();
    Serial.println("Current WiFi Status Code: ");
    Serial.println(WiFi.status());
    Serial.println("  0=IDLE, 1=NO_SSID, 3=CONNECTED, 4=FAILED, 6=DISCONNECTED");
    Serial.println();
    
    display.clearDisplay();
    display.setCursor(0, 32);
    display.println("WiFi FAILED!");
    display.println();
    display.println("Check settings");
    display.print("Status: ");
    display.println(WiFi.status());
    display.display();
    
    // Continue without WiFi for sensor testing
    Serial.println("Continuing without WiFi...");
    Serial.println("Sensors will still work locally.");
  }

  // Start Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  display.clearDisplay();
  display.setCursor(0, 32);
  display.println("Connecting to");
  display.println("Firebase...");
  display.display();

  // Sign up anonymously (Test Mode allows this)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✓ Firebase Auth Successful");
    signupOK = true;
  } else {
    Serial.printf("✗ Firebase Error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("\n========== SYSTEM READY ==========");
  Serial.println();
}

// ================= 4. MAIN LOOP =================
void loop() {
  // Update every 1 second (1000ms)
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

#if BYPASS_SENSORS
    // ============ BYPASS MODE: Read FROM Firebase ============
    float temp = 25.0;      // defaults
    float humid = 50.0;
    int moistPercent = 50;
    float lux = 500.0;

    Serial.print("Reading from Firebase... ");
    
    if (Firebase.RTDB.getJSON(&fbdo, "/plants/gaia_01")) {
      FirebaseJson &json = fbdo.jsonObject();
      FirebaseJsonData jsonData;
      
      if (json.get(jsonData, "temperature")) temp = jsonData.floatValue;
      if (json.get(jsonData, "soil_moisture")) moistPercent = jsonData.intValue;
      if (json.get(jsonData, "light_intensity")) lux = jsonData.floatValue;
      if (json.get(jsonData, "humidity")) humid = jsonData.floatValue;
      
      Serial.println("OK");
      Serial.print("  Temp: "); Serial.print(temp);
      Serial.print("C | Humid: "); Serial.print(humid);
      Serial.print("% | Soil: "); Serial.print(moistPercent);
      Serial.print("% | Light: "); Serial.print(lux);
      Serial.println(" lux");
    } else {
      Serial.print("FAILED: ");
      Serial.println(fbdo.errorReason());
    }

    // Update display with Firebase values
    updateScreen(temp, moistPercent, lux);

#else
    // ============ NORMAL MODE: Read sensors, upload to Firebase ============
    // --- STEP A: READ SENSORS ---
    float temp = dht.readTemperature();
    float humid = dht.readHumidity();
    int rawMoisture = analogRead(MOISTURE_PIN);
    float lux = lightMeter.readLightLevel();

    // Convert Moisture to Percentage
    int moistPercent = map(rawMoisture, DRY_VAL, WET_VAL, 0, 100);
    moistPercent = constrain(moistPercent, 0, 100);

    // Check for sensor error
    if (isnan(temp) || isnan(humid)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // --- DISPLAY ON OLED ---
    updateScreen(temp, moistPercent, lux);

    // --- STEP B: UPLOAD TO FIREBASE ---
    Serial.print("Sending to Firebase... ");

    FirebaseJson json;
    json.set("temperature", temp);
    json.set("humidity", humid);
    json.set("soil_moisture", moistPercent);
    json.set("soil_raw", rawMoisture);
    json.set("light_intensity", lux >= 0 ? lux : 0);
    json.set("timestamp", millis());

    if (Firebase.RTDB.setJSON(&fbdo, "/plants/gaia_01", &json)) {
      Serial.println("SUCCESS! Data saved.");
      Serial.print("Temp: "); Serial.print(temp);
      Serial.print("°C | Humid: "); Serial.print(humid);
      Serial.print("% | Soil: "); Serial.print(moistPercent);
      Serial.print("% | Light: "); Serial.print(lux);
      Serial.println(" lux");
    } else {
      Serial.print("FAILED: ");
      Serial.println(fbdo.errorReason());
    }
#endif
  }
}