# Project Gaia — Device Firmware 🌿🤖

**The nervous system of your botanical companion.**

This repository contains the C++ firmware for the ESP32 microcontroller that powers the physical Gaia device. It handles real-time environmental sensor reading, an expressive OLED "Living Interface" that reacts to your plant's condition, and bi-directional synchronization with Firebase Realtime Database.

> **📱 Companion App Required:** This device is designed to be paired with the **[Project Gaia Flutter App](https://github.com/HoogaBoga/project_gaia)**. The app and device communicate through a shared Firebase Realtime Database — the device uploads sensor data, and the app writes back species-specific care thresholds. **You need both repositories to run the full system.**

---

## 📱 How This Device Works With the Companion App

Project Gaia is a two-part system. This firmware is the **hardware half**; the [Flutter app](https://github.com/HoogaBoga/project_gaia) is the **software half**. They communicate through Firebase in real time:

```
┌───────────────────┐                              ┌──────────────────────┐
│   ESP32 Device    │         Firebase RTDB        │   Flutter App        │
│   (this repo)     │                              │   (companion repo)   │
│                   │                              │                      │
│  Reads sensors:   │  ── uploads every 1s ──────▶ │  Streams live data   │
│  • Temperature    │     /plants/gaia_01/         │  to dashboard with   │
│  • Humidity       │     { temperature, humidity, │  health rings, HP    │
│  • Soil Moisture  │       soil_moisture,         │  bar, and digital    │
│  • Light (Lux)    │       light_intensity }      │  twin image          │
│                   │                              │                      │
│  Fetches every    │  ◀── writes thresholds ───── │  Identifies species  │
│  30 seconds:      │     /plants/gaia_01/         │  via camera + Gemini │
│  • Species name   │     thresholds/              │  AI, then generates  │
│  • Care ranges    │     { species, moisture_low, │  optimal care ranges │
│                   │       temp_high, ... }       │  for that species    │
│                   │                              │                      │
│  Displays OLED    │                              │  Also provides:      │
│  face based on    │                              │  • AI chatbot        │
│  dynamic ranges   │                              │  • Health analysis   │
│                   │                              │  • Smart alerts      │
│                   │                              │  • AI pixel-art twin │
└───────────────────┘                              └──────────────────────┘
```

### What the Companion App Does

The [Project Gaia Flutter App](https://github.com/HoogaBoga/project_gaia) provides the full mobile experience:

| Feature | Description |
| --- | --- |
| **Real-Time Dashboard** | Streams sensor data from this device and displays it with an HP bar and animated digital twin image |
| **Health Stats** | Four concentric ring visualizations for water, humidity, sunlight, and temperature with overall health % |
| **AI Digital Twin** | Generates three pixel-art plant avatars (healthy / warning / critical) using Google Gemini, with backgrounds removed via remove.bg |
| **Plant Chatbot** | "Ask Gaia" — chat with your plant in-character (Friendly, Calm, Energetic, Wise, or Playful personality) |
| **Camera Plant ID** | During onboarding, snap a photo and Gemini identifies the species automatically |
| **Species Thresholds** | Gemini generates species-specific optimal care ranges (temp, humidity, moisture, light) and writes them to Firebase — **this is what the ESP32 reads** to adapt its OLED faces |
| **Smart Notifications** | Alerts when conditions go outside the species-specific optimal ranges |
| **AI Health Analysis** | The plant "speaks" about its current condition using real sensor numbers |

### Setup Order

1. **Set up Firebase** — Create a project with Realtime Database and Storage enabled (both repos share the same Firebase project)
2. **Flash this firmware** — Configure WiFi and Firebase credentials in `main.cpp`, upload to ESP32
3. **Set up the Flutter app** — Follow the [app README](https://github.com/HoogaBoga/project_gaia#getting-started) to configure API keys and Firebase
4. **Onboard a plant** — Open the app, take a photo to identify the species → the app writes thresholds to Firebase → the ESP32 picks them up within 30 seconds and the OLED face logic adapts

---

## 🧠 Hardware Architecture

The device is built around the **ESP32**, chosen for its dual-core processing power and built-in Wi-Fi capabilities. It connects a suite of environmental sensors to monitor the plant's health and displays real-time reactions on a 128×64 OLED screen.

### Component List & Pin Mapping

| Component | ESP32 Pin | I2C Address | Purpose |
| --- | --- | --- | --- |
| **SSD1306 OLED Display** | GPIO 21 (SDA), GPIO 22 (SCL) | `0x3C` (fallback `0x3D`) | Shows emotional faces, status bar with WiFi/battery/species name |
| **BH1750 Light Sensor** | GPIO 21 (SDA), GPIO 22 (SCL) | `0x23` (ADDR→GND) | Measures ambient light intensity in lux |
| **DHT22 Sensor** | GPIO 4 | — | Measures air temperature (°C) and relative humidity (%) |
| **Capacitive Soil Moisture** | GPIO 34 (ADC1) | — | Measures soil moisture via analog reading, mapped to 0–100% |

> **Note:** The OLED and BH1750 share the I2C bus. GPIO 34 is on ADC1, which is safe to use while WiFi is active (ADC2 pins conflict with WiFi on the ESP32).

### Wiring Diagram

```
ESP32 Dev Board
┌──────────────┐
│           3V3 ├──── VCC (OLED, BH1750, DHT22)
│           GND ├──── GND (all sensors)
│       GPIO 21 ├──── SDA (OLED + BH1750, shared I2C)
│       GPIO 22 ├──── SCL (OLED + BH1750, shared I2C)
│        GPIO 4 ├──── DATA (DHT22, with 10kΩ pull-up to 3V3)
│       GPIO 34 ├──── AOUT (Capacitive Soil Moisture Sensor)
└──────────────┘
```

---

## 🛠️ Key Features

### 1. The "Living" Interface (OLED)

Gaia translates raw sensor data into **9 emotional states** drawn with pixel-level primitives on the 128×64 OLED. The firmware uses a **priority system** to decide which face to display — more critical conditions always take precedence. **Thresholds are dynamic** and adapt to the plant's species via Firebase.

| Priority | Condition | Face | ASCII | Trigger |
| --- | --- | --- | --- | --- |
| 1 (Critical) | Soil too dry | Thirsty | `X _ X` + teardrop | `soil_moisture < moisture_low` |
| 1 (Critical) | Soil too wet | Overwatered | `@ _ @` + sweat drops | `soil_moisture > moisture_high` |
| 2 (Warning) | Too hot | Hot | `> _ <` + heat waves | `temperature > temp_high` |
| 2 (Warning) | Too cold | Cold | `O _ O` + shiver lines | `temperature < temp_low` |
| 3 (Warning) | Air too humid | Humid | `_ _ _` + droplets | `humidity > humidity_high` |
| 3 (Warning) | Air too dry | Dry Air | `O _ O` + cracks | `humidity < humidity_low` |
| 4 (Minor) | Too dark | Dark | `- _ -` + Zzz | `light < lux_low` |
| 4 (Minor) | Too bright | Bright | `= _ =` + sun rays | `light > lux_high` |
| — (Default) | All good | Happy | `^ _ ^` + smile | All values in range |

The **status bar** at the top of the OLED shows:
- **Left:** WiFi icon (connected or disconnected)
- **Center:** Plant species name (from Firebase, e.g. "Boston Fern") — truncated to 14 characters
- **Right:** Battery indicator

### 2. Firebase Synchronization

The device operates as a real-time IoT node with two data flows:

**Uploads (every 1 second):**

| Field | Type | Example | Description |
| --- | --- | --- | --- |
| `temperature` | float | `25.5` | DHT22 reading in °C |
| `humidity` | float | `60.0` | DHT22 reading in % |
| `soil_moisture` | int | `45` | Calibrated percentage (0–100%) |
| `soil_raw` | int | `2400` | Raw ADC reading (for debugging) |
| `light_intensity` | float | `500.0` | BH1750 reading in lux |
| `timestamp` | int | `1700000000` | `millis()` uptime |

**Downloads (every 30 seconds):**

Species-specific thresholds from `/plants/gaia_01/thresholds/` — written by the companion Flutter app after it identifies the plant species using Gemini AI.

### 3. Dynamic Species-Based Thresholds 🌱

Every plant species has different needs. A cactus thrives at 15% soil moisture while a fern would be dying. The ESP32 pulls species-specific thresholds from Firebase so the OLED faces react appropriately for the actual plant being monitored.

**The full flow across both repositories:**

1. **User onboards in the app** — takes a photo of their plant
2. **Gemini AI identifies the species** (e.g., "Boston Fern") in the Flutter app
3. **Gemini generates a care profile** — optimal ranges for temperature, humidity, soil moisture, and light for that species
4. **The app writes thresholds to Firebase** at `/plants/gaia_01/thresholds/`
5. **The ESP32 fetches these thresholds** on startup and re-syncs every 30 seconds via `fetchThresholdsFromFirebase()`
6. **The OLED face logic uses the dynamic thresholds** — so a cactus won't show a thirsty face at 20% moisture, but a fern will
7. **The species name appears on the OLED status bar** (centered between WiFi and battery icons)

If thresholds haven't been set yet (first boot, or Firebase is unreachable), the firmware falls back to sensible defaults for common houseplants:

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
| `species` | `"Unknown"` | Displayed on OLED status bar |

---

## 📁 Project Structure

```
PROJECT-GAIA-FIREBASE/
├── platformio.ini          # PlatformIO config (board, libs, baud rate)
├── src/
│   └── main.cpp            # All firmware logic (661 lines)
│                            #   ├── User configuration (WiFi, Firebase, pins)
│                            #   ├── PlantThresholds struct + Firebase fetch
│                            #   ├── OLED face drawing (9 states, pixel primitives)
│                            #   ├── Status bar (WiFi icon, species name, battery)
│                            #   ├── setup() — I2C scan, sensor init, WiFi, Firebase
│                            #   └── loop() — read sensors, upload, sync thresholds, draw face
├── include/
│   └── bitmaps.h           # WiFi icons (PROGMEM bitmaps) + face type constants
├── lib/                    # Custom libraries (empty — all deps from registry)
└── test/                   # Unit tests directory
```

---

## 🚀 Setup & Configuration

### Prerequisites

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- An ESP32 dev board (e.g., ESP32-WROOM-32 or ESP32-DevKitC)
- The sensors listed in the [Hardware Architecture](#-hardware-architecture) section
- A Firebase project with **Realtime Database** enabled
- A 2.4GHz WiFi network (ESP32 does not support 5GHz)
- The **[Project Gaia Flutter App](https://github.com/HoogaBoga/project_gaia)** (for species identification and threshold management)

### 1. Clone & Open

```bash
git clone https://github.com/mjtagaan/project_gaia_device.git
cd project_gaia_device
```

Open the project in VS Code with the PlatformIO extension, or in PlatformIO IDE.

### 2. Library Dependencies

All dependencies are declared in `platformio.ini` and install automatically on first build:

| Library | Version | Purpose |
| --- | --- | --- |
| `Firebase Arduino Client Library` (Mobizt) | ^4.4.14 | Firebase RTDB communication |
| `DHT sensor library` (Adafruit) | ^1.4.6 | DHT22 temperature & humidity |
| `Adafruit Unified Sensor` | ^1.1.14 | Sensor abstraction layer |
| `BH1750` (claws) | ^1.3.0 | I2C light sensor driver |
| `Adafruit SSD1306` | ^2.5.7 | OLED display driver |
| `Adafruit GFX Library` | ^1.11.3 | Graphics primitives for OLED |

### 3. Configuration

Open `src/main.cpp` and update the **User Configuration** section at the top:

```cpp
// ================= 1. USER CONFIGURATION =================

// WIFI SETTINGS
#define WIFI_SSID "YOUR_WIFI_NAME"        // Must be 2.4GHz
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// FIREBASE SETTINGS
#define API_KEY "YOUR_FIREBASE_WEB_API_KEY"
#define DATABASE_URL "https://your-project-id-default-rtdb.firebaseio.com/"
```

> **Important:** The `DATABASE_URL` and `API_KEY` must match the same Firebase project used by the [companion Flutter app](https://github.com/HoogaBoga/project_gaia). Both the device and the app read/write to `/plants/gaia_01`.

### 4. Firebase Setup

1. Go to **[Firebase Console](https://console.firebase.google.com/)** → Create a project (or use the same one as the Flutter app).
2. **Project Settings** → **General** → Copy your **Web API Key**.
3. **Build** → **Realtime Database** → Create database → Copy the URL.
4. Set **Database Rules** for testing:
   ```json
   {
     "rules": {
       ".read": true,
       ".write": true
     }
   }
   ```
   > For production, restrict write access to authenticated users and limit read paths.

### 5. Sensor Calibration

You **must** calibrate the capacitive soil moisture sensor for your specific soil type. Raw ADC values vary between sensors and soil compositions.

1. Power on the ESP32 with the sensor in **dry air**. Open the Serial Monitor (`115200` baud) and note the `soil_raw` value. This is your `DRY_VAL`.
2. Insert the sensor into a **cup of water** (not past the line). Note the `soil_raw` value. This is your `WET_VAL`.
3. Update the values in `main.cpp`:

```cpp
const int DRY_VAL = 3500;  // Replace with your 'Air' reading
const int WET_VAL = 1200;  // Replace with your 'Water' reading
```

The firmware maps: `DRY_VAL` → 0% moisture, `WET_VAL` → 100% moisture.

### 6. Build & Upload

```bash
# PlatformIO CLI
pio run --target upload

# Then open the serial monitor
pio device monitor --baud 115200
```

Or use the PlatformIO sidebar in VS Code: **Build** → **Upload** → **Monitor**.

---

## 📊 Firebase Data Structure

The device reads/writes to the `/plants/gaia_01` path in Firebase Realtime Database:

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
      "profile": {
        "name": "Fern",
        "species": "Boston Fern",
        "personality": "Friendly"
      },
      "thresholds": {
        "species": "Boston Fern",
        "moisture_low": 40,
        "moisture_high": 80,
        "temp_low": 15.0,
        "temp_high": 28.0,
        "humidity_low": 50.0,
        "humidity_high": 80.0,
        "lux_low": 200.0,
        "lux_high": 800.0
      },
      "visuals": {
        "perfect": { "imageUrl": "...", "timestamp": "..." },
        "warning": { "imageUrl": "...", "timestamp": "..." },
        "critical": { "imageUrl": "...", "timestamp": "..." }
      }
    }
  }
}
```

| Path | Written by | Read by | Description |
| --- | --- | --- | --- |
| `/plants/gaia_01/temperature`, `humidity`, etc. | **ESP32** | **Flutter app** | Live sensor data, uploaded every 1 second |
| `/plants/gaia_01/thresholds/` | **Flutter app** | **ESP32** | Species-specific care ranges from Gemini AI |
| `/plants/gaia_01/profile/` | **Flutter app** | **Flutter app** | Plant name, species, personality |
| `/plants/gaia_01/visuals/` | **Flutter app** | **Flutter app** | AI-generated pixel-art avatar URLs |

> **The ESP32 never writes to `thresholds`, `profile`, or `visuals`** — it only uploads sensor readings and reads thresholds.

---

## 🔄 Firmware Boot Sequence

```
1. Initialize Serial (115200 baud)
2. Initialize I2C bus (GPIO 21/22) + scan for connected devices
3. Initialize OLED (try 0x3C, fallback 0x3D)
   └── Show "GAIA System — Initializing..."
4. Initialize BH1750 light sensor
5. Initialize DHT22 + soil moisture pin
6. Connect to WiFi (up to 50 attempts, retry every 10)
   └── Show progress on OLED
7. Authenticate with Firebase (anonymous sign-up)
8. Fetch species thresholds from /plants/gaia_01/thresholds/
9. Print species name + threshold summary to Serial
10. Enter main loop:
    ├── Read all 4 sensors
    ├── Every 30s: re-fetch thresholds from Firebase
    ├── Determine face from priority rules + dynamic thresholds
    ├── Draw status bar + face on OLED
    └── Upload sensor JSON to Firebase (every 1s)
```

---

## ⚠️ Troubleshooting

| Problem | Solution |
| --- | --- |
| **OLED blank / not working** | Check wiring (SDA→21, SCL→22, VCC→3V3). The firmware tries both `0x3C` and `0x3D` addresses automatically. Check Serial Monitor for I2C scan output. |
| **Firebase connection fails** | Verify `API_KEY` and `DATABASE_URL` match your Firebase project. Ensure Realtime Database rules allow read/write. Check that "Email/Password" sign-in provider is enabled in Firebase Authentication. |
| **WiFi won't connect** | ESP32 only supports **2.4GHz** networks — 5GHz will not work. Check SSID/password. Move the board closer to the router. Serial Monitor shows status codes. |
| **Soil moisture reads 0% or 100% always** | You need to [calibrate](#5-sensor-calibration) `DRY_VAL` and `WET_VAL` for your specific sensor and soil. |
| **BH1750 returns -1 or -2** | Check that the BH1750 `ADDR` pin is connected to **GND** (for address `0x23`). Verify I2C wiring. |
| **OLED shows "Unknown" for species** | The companion app hasn't been set up yet, or hasn't identified a plant. Open the [Flutter app](https://github.com/HoogaBoga/project_gaia), complete onboarding, and the species name + thresholds will appear within 30 seconds. |
| **Faces don't match plant needs** | The default thresholds are generic houseplant values. Pair with the [companion app](https://github.com/HoogaBoga/project_gaia) to get species-specific ranges from Gemini AI. |

---

## 🔗 Related

- **[Project Gaia — Flutter Companion App](https://github.com/HoogaBoga/project_gaia)** — Real-time dashboard, AI digital twin, chatbot, and species identification
- **[PlatformIO Documentation](https://docs.platformio.org/)** — Build system used for this project
- **[Firebase Arduino Client Library](https://github.com/mobizt/Firebase-ESP-Client)** — Firebase RTDB library by Mobizt
