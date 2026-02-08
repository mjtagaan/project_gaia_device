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

Gaia translates raw sensor data into **7 emotional states**. The firmware uses a priority system to decide which face to display (e.g., *Thirst* overrides *Cold*).

* **Happy (`^ _ ^`):** Conditions are optimal.
* **Thirsty (`X _ X`):** Moisture < 30%. (Priority 1)
* **Overwatered (`@ _ @`):** Moisture > 85%. (Priority 1)
* **Hot (`> _ <`):** Temp > 30°C. (Priority 2)
* **Cold (`O _ O`):** Temp < 15°C. (Priority 2)
* **Dark (`- _ -`):** Light < 100 Lux. (Priority 3)
* **Bright (`= _ =`):** Light > 2000 Lux. (Priority 3)

### 2. Firebase Synchronization

The device acts as a real-time node.

* **Uploads:** Temperature, Humidity, Soil Moisture, Light Intensity, and Timestamp.
* **Downloads:** (In Bypass Mode) Virtual sensor values for testing.

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
      "timestamp": 1700000000
    }
  }
}

```

## ⚠️ Troubleshooting

* **OLED not working?** Check if your display address is `0x3C` or `0x3D`. The code attempts both automatically.
* **Firebase Error?** Ensure "Email/Password" sign-in provider is enabled in Firebase Authentication, or that your Database Rules allow anonymous read/write.
* **WiFi Fails?** The ESP32 only supports 2.4GHz WiFi networks. 5GHz will not work.
