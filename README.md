# Project Gaia - Device Firmware 🌿🤖

**The nervous system of your botanical companion.**

This repository contains the C++ firmware for the ESP32 microcontroller that powers the physical Gaia device. It handles real-time sensor data acquisition, local OLED visualization (The "Living Interface"), and bi-directional synchronization with Firebase.

## 🧠 Hardware Architecture

The device is built around the **ESP32**, chosen for its dual-core processing power and built-in Wi-Fi/Bluetooth capabilities. It connects a suite of environmental sensors to monitor the plant's health.

### Component List & Pin Mapping

| Component | ESP32 Pin | Variable Name | Notes |
| --- | --- | --- | --- |
| **OLED Display (I2C)** | GPIO 21 (SDA), 22 (SCL) | `0x3C` Address | Shows faces & status |
| **BH1750 Light Sensor** | GPIO 21 (SDA), 22 (SCL) | `0x23` Address | Measures Lux |
| **DHT22 Sensor** | GPIO 4 | `DHTPIN` | Temp & Humidity |
| **Soil Moisture** | GPIO 34 (Analog) | `MOISTURE_PIN` | Capacitive Sensor |

---

## 🛠️ Key Features

### 1. The "Living" Interface (OLED)

Gaia translates raw sensor data into **9 emotional states**. The firmware uses a priority system to decide which face to display (e.g., *Thirst* overrides *Cold*). **Thresholds are dynamic** — they adapt to the plant's species via Firebase (see below).

* **Happy (`^ _ ^`):** All conditions are within range.
* **Thirsty (`X _ X`):** Moisture too low. (Priority 1 — Critical)
* **Overwatered (`@ _ @`):** Moisture too high. (Priority 1 — Critical)
* **Hot (`> _ <`):** Temperature too high. (Priority 2 — Warning)
* **Cold (`O _ O`):** Temperature too low. (Priority 2 — Warning)
* **Humid (`_ _ _` + droplets):** Humidity too high. (Priority 3 — Warning)
* **Dry Air (`O _ O` + cracks):** Humidity too low. (Priority 3 — Warning)
* **Dark (`- _ -`):** Light too low. (Priority 4 — Minor)
* **Bright (`= _ =`):** Light too high. (Priority 4 — Minor)

### 2. Firebase Synchronization

The device acts as a real-time node.

* **Uploads:** Temperature, Humidity, Soil Moisture, Light Intensity, and Timestamp.
* **Downloads:** Species-specific thresholds from `/plants/gaia_01/thresholds/` (synced every 30 seconds).

### 3. Dynamic Species-Based Thresholds 🌱

Every plant species has different needs. A cactus thrives at 15% soil moisture while a fern would be dying. The ESP32 pulls species-specific thresholds from Firebase so the OLED faces react appropriately for the actual plant being monitored.

**How it works:**

1. The **Flutter companion app** uses camera vision to identify the plant species.
2. The app writes optimal thresholds for that species to Firebase at `/plants/gaia_01/thresholds/`.
3. The **ESP32 fetches these thresholds** on startup and re-syncs every 30 seconds.
4. The OLED face logic uses the dynamic thresholds instead of hardcoded values.
5. The detected **species name is displayed** in the OLED status bar (centered between WiFi and battery icons).

If thresholds haven't been set yet (or Firebase is unreachable), the firmware falls back to sensible defaults for common houseplants.

| Threshold | Default | Description |
| --- | --- | --- |
| `moisture_low` | 30% | Below → Thirsty face |
| `moisture_high` | 85% | Above → Overwatered face |
| `temp_low` | 15°C | Below → Cold face |
| `temp_high` | 30°C | Above → Hot face |
| `humidity_low` | 30% | Below → Dry Air face |
| `humidity_high` | 80% | Above → Humid face |
| `lux_low` | 100 lux | Below → Dark face |
| `lux_high` | 2000 lux | Above → Bright face |
| `species` | "Unknown" | Displayed on OLED status bar |

---

## 🚀 Setup & Configuration

### 1. Library Dependencies

Install these libraries via Arduino IDE or PlatformIO:

* `Firebase Arduino Client Library for ESP8266 and ESP32` (Mobizt)
* `Adafruit GFX Library`
* `Adafruit SSD1306`
* `DHT sensor library`
* `BH1750` (claws)

### 2. Configuration (`User Configuration` Section)

Locate the top section of `main.cpp` and update these values:

```cpp
// WIFI SETTINGS
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// FIREBASE SETTINGS
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "https://your-project-id.firebaseio.com/"

```

### 3. Firebase Initialization

1. Go to **Firebase Console** > **Project Settings**.
2. Under **General**, copy your `Web API Key`.
3. Under **Realtime Database**, create a database and copy the URL.
4. **Rules:** For testing, set your database rules to public (or set up proper Auth later):
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}

```



### 4. Sensor Calibration

You **must** calibrate the capacitive soil sensor for your specific soil type.

1. Run the code with the sensor in dry air. Note the `rawMoisture` value (printed in Serial). Set this as `DRY_VAL`.
2. Submerge the sensor in a cup of water. Note the `rawMoisture`. Set this as `WET_VAL`.

```cpp
// Example Calibration in code
const int DRY_VAL = 3500; // Replace with your 'Air' reading
const int WET_VAL = 1200; // Replace with your 'Water' reading

```

---

## 📊 Data Structure

The device reads/writes to the `/plants/gaia_01` path in Firebase:

```json
{
  "plants": {
    "gaia_01": {
      "temperature": 25.5,
      "humidity": 60.0,
      "soil_moisture": 45,
      "soil_raw": 2400,
      "light_intensity": 500,
      "timestamp": 1700000000,
      "thresholds": {
        "species": "Cactus",
        "moisture_low": 10,
        "moisture_high": 40,
        "temp_low": 5.0,
        "temp_high": 40.0,
        "humidity_low": 10.0,
        "humidity_high": 50.0,
        "lux_low": 500.0,
        "lux_high": 50000.0
      }
    }
  }
}

```

> **Note:** The `thresholds` object is written by the Flutter companion app when a plant species is identified. The ESP32 only *reads* from this path — it never overwrites it.


## ⚠️ Troubleshooting

* **OLED not working?** Check if your display address is `0x3C` or `0x3D`. The code attempts both automatically.
* **Firebase Error?** Ensure "Email/Password" sign-in provider is enabled in Firebase Authentication, or that your Database Rules allow anonymous read/write.
* **WiFi Fails?** The ESP32 only supports 2.4GHz WiFi networks. 5GHz will not work.
