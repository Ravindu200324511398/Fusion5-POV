# Fusion 5 — POV Hologram Fan Controller

> Persistence-of-vision hologram display system powered by ESP32-S3 · 200 APA102 LEDs · Dual-fan vertical split

---

## Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Wiring Diagram](#wiring-diagram)
4. [Software & Libraries](#software--libraries)
5. [Arduino Setup & Flash](#arduino-setup--flash)
6. [WiFi Configuration](#wifi-configuration)
7. [Web Dashboard](#web-dashboard)
8. [Clock & Text Display](#clock--text-display)
9. [Live Streaming (Python Sender)](#live-streaming-python-sender)
10. [OTA Firmware Update](#ota-firmware-update)
11. [File Structure](#file-structure)
12. [Troubleshooting](#troubleshooting)

---

## Overview

Fusion 5 is a dual-blade POV (Persistence of Vision) hologram fan system. A single APA102 LED strip with 200 LEDs spans a spinning blade, and a hall-effect sensor triggers precise angular rendering — creating vivid floating hologram images in mid-air.

**Key features:**
- 200 APA102 LEDs (100 per half-blade)
- 320 angular divisions per rotation for smooth images
- Double-buffered JPEG frame decoding
- Wi-Fi web dashboard for media upload and control
- Live screen streaming from PC via Python sender
- Analog clock, digital clock, and custom text display on the fan
- OTA (Over-the-Air) firmware updates
- Dual-fan vertical split support (Top 0–60% / Bottom 40–100% with 20% overlap)

---

## Hardware Requirements

| Component | Details |
|---|---|
| Microcontroller | ESP32-S3 with PSRAM |
| LED Strip | APA102 (DotStar) — 200 LEDs total |
| Hall Sensor | Any digital hall-effect sensor (e.g. 3144) |
| Power Supply | 5V, adequate current for 200 LEDs (min 5A recommended) |
| SPI Data Pin | GPIO 11 (MOSI) |
| SPI Clock Pin | GPIO 12 (SCLK) |
| Hall Sensor Pin | GPIO 4 |
| Flash Size | 4MB minimum (custom partition required) |
| PSRAM | Required for double-frame buffer |

---

## Wiring Diagram

```
ESP32-S3                APA102 Strip
─────────               ────────────
GPIO 11  ──────────────  DATA  (DI)
GPIO 12  ──────────────  CLOCK (CI)
GND      ──────────────  GND
5V PSU   ──────────────  VCC

ESP32-S3                Hall Sensor
─────────               ───────────
GPIO 4   ──────────────  OUT (signal)
3.3V     ──────────────  VCC
GND      ──────────────  GND
```

> **Note:** The LED strip uses hardware SPI (FSPI). MISO is GPIO 13 (unused), CS is GPIO 10 (unused). Only DATA and CLOCK are connected to the strip.

---

## Software & Libraries

### Arduino Libraries (install via Library Manager)

| Library | Purpose |
|---|---|
| `NeoPixelBus` | APA102 DotStar LED control |
| `ArduinoJson` | JSON API responses |
| `JPEGDEC` | JPEG decoding for frames |
| `ArduinoOTA` | Over-the-air firmware updates |
| `ESPmDNS` | mDNS hostname resolution |
| `LittleFS` | Internal filesystem for media storage |

### Python Dependencies (for PC sender)

```bash
pip install customtkinter numpy mss opencv-python requests
```

---

## Arduino Setup & Flash

### 1. Board Settings (Arduino IDE)

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash Size | 4MB |
| Partition Scheme | Use `partitions.csv` (custom) |
| PSRAM | OPI PSRAM |
| Upload Speed | 921600 |

### 2. Custom Partition

Use the included `partitions.csv` file. Place it in the same folder as the `.ino` file. This allocates space for LittleFS media storage.

### 3. Flash Steps

1. Open `bbPOV_router_LittleFS_format_fixed.ino` in Arduino IDE
2. Select your ESP32-S3 board and COM port
3. Ensure `partitions.csv` and `webpage.h` are in the same sketch folder
4. Click **Upload**
5. Open Serial Monitor at **115200 baud** to watch boot logs

---

## WiFi Configuration

The device tries **Router mode** first, then falls back to **AP mode**.

### Router Mode (default)
Edit these lines in the `.ino` file before flashing:

```cpp
const char *routerSsid = "YourWiFiName";
const char *routerPass = "YourWiFiPassword";
```

On successful connection, the IP address is printed to Serial Monitor. Open that IP in a browser to access the dashboard.

### AP Fallback Mode
If router connection fails, the device creates its own hotspot:

| Setting | Value |
|---|---|
| SSID | `bbPOV-Fan` |
| Password | `12345678` |
| IP Address | `192.168.4.1` |

Connect your phone or PC to that hotspot, then open `http://192.168.4.1` in a browser.

---

## Web Dashboard

Open the device IP in any browser to access the **Fusion 5 Hologram Controller** dashboard.

### Library Panel
- Lists all uploaded animations stored on the device
- **Play / Pause** — start or stop the fan display
- **Previous / Next** — cycle through animations
- **Auto-next** — automatically advance to next animation when current ends
- **Delete** — remove an animation from storage
- Shows live RPM, free heap, and flash storage

### New Animation Panel
- Drag & drop or select an image (PNG/JPG) or video (MP4/WebM)
- Set animation name, edge brightness, and center brightness
- Automatically converts to polar format and uploads to the fan
- Live hologram preview shows exactly what the fan will display

### API Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | Web dashboard |
| `/status` | GET | JSON status (RPM, playing, media, storage) |
| `/list` | GET | JSON array of available animations |
| `/select?name=X` | GET | Switch to animation X |
| `/delete?name=X` | GET | Delete animation X |
| `/playpause` | GET | Toggle play/pause |
| `/autonext` | GET | Toggle auto-next |
| `/next` | GET | Next animation |
| `/prev` | GET | Previous animation |
| `/reboot` | GET | Reboot the device |
| `/upload_file` | POST | Upload a JPEG frame |
| `/upload_end` | POST | Finalize upload |
| `/update` | GET/POST | Firmware OTA update page |

---

## Clock & Text Display

The dashboard includes a **Clock & Text Display** panel with live clocks and direct-to-fan rendering.

### Analog Clock
- Live canvas clock updates every second in the browser
- Cyan face, white hour hand, cyan minute hand, magenta second hand

### Digital Clock
- Live `HH:MM:SS` display with full date
- Gradient display in JetBrains Mono font

### Send to Fan Buttons

| Button | Animation Name | Description |
|---|---|---|
| **Send Text** | `fusion5_text` | Renders custom text in chosen font/color on the fan |
| **Send Analog Clock** | `fusion5_analog` | Sends current analog clock face with Fusion 5 label |
| **Send Digital Clock** | `fusion5_digital` | Sends digital time + date display with glow effect |

**How to use:**
1. Type your text in the input field (default: `FUSION 5`)
2. Choose font style, text color, and background color
3. Click any **Send** button — it converts to polar format and uploads automatically
4. The POV preview circle shows exactly what the fan will display
5. The animation appears in the Library and starts playing immediately

---

## Live Streaming (Python Sender)

The Python sender streams your screen live to one or two fans in real time.

### Run the Sender

```bash
cd "Fusion5_POV_COMPLETE/bbPOV_custom_build 4/API"
python sender_dual_vertical_overlap20_MODERN.py
```

### Configuration

| Setting | Default | Description |
|---|---|---|
| Top Fan IP | `192.168.8.168` | IP address of Fan 1 (top half) |
| Bottom Fan IP | `192.168.8.191` | IP address of Fan 2 (bottom half) |
| Port | `22333` | TCP streaming port |
| Capture Source | Screen | What to capture and stream |
| Brightness | 10% | Edge brightness of streamed image |
| Center Brightness | 10% | Center LED brightness |
| Target FPS | 12 | Streaming frame rate |

### Image Mapping (Dual Fan)
```
Screen capture area
┌─────────────────┐
│  TOP → Fan 1    │  0% – 60%
│  ─ ─ ─ ─ ─ ─   │  (dashed = 20% overlap zone)
│  BOT → Fan 2    │  40% – 100%
└─────────────────┘
```

### Steps
1. Enter the IP addresses of your fan(s)
2. Adjust brightness and FPS sliders
3. Position the capture area overlay on screen
4. Click **Start Streaming**
5. Click **Stop** to return to stored media playback

---

## OTA Firmware Update

Update firmware wirelessly — no USB cable needed.

### Via Web Dashboard
1. Open the dashboard in your browser
2. Click **Firmware** button
3. Select your new `.bin` file
4. Click **Upload Firmware** — device reboots automatically

### Via Arduino IDE (OTA)
1. The device appears as a network port in Arduino IDE after first boot
2. Select the network port and upload normally

---

## File Structure

```
Fusion5_POV_COMPLETE/
├── README_FUSION5.md                          ← This file
├── bbPOV_custom_build 4/
│   ├── API/
│   │   └── sender_dual_vertical_overlap20_MODERN.py   ← PC live streaming app
│   ├── bbPOV_router_dual_fan_WIFI_FIXED/
│   │   └── bbPOV_router_LittleFS_format_fixed/
│   │       ├── bbPOV_router_LittleFS_format_fixed.ino ← Main firmware
│   │       ├── webpage.h                               ← Web dashboard (HTML/CSS/JS)
│   │       └── partitions.csv                          ← Custom flash partition table
│   ├── QUALITY_CHANGES.txt
│   ├── QUALITY_OTA_CHANGES.txt
│   └── OTA_UPDATE_README.txt
└── README_4MB_FIX.txt
```

### Media Storage on Device (LittleFS)
```
/bbPOV-P/
├── your_animation/
│   ├── 0.jpg
│   ├── 1.jpg
│   └── ...
├── fusion5_text/
│   └── 0.jpg       ← Generated by Clock & Text panel
├── fusion5_analog/
│   └── 0.jpg
└── fusion5_digital/
    └── 0.jpg
```

---

## Troubleshooting

| Problem | Solution |
|---|---|
| LEDs don't light up on boot | Check DATA (GPIO 11) and CLOCK (GPIO 12) wiring. Ensure 5V power is sufficient. |
| "LittleFS mount failed" in Serial | Device auto-formats. Wait for reboot. If persistent, re-flash with correct partition scheme. |
| Dashboard not loading | Confirm IP from Serial Monitor. Try `http://192.168.4.1` if in AP mode. |
| Images upload but don't display | Check hall sensor on GPIO 4. Fan must be spinning for POV to work. |
| Poor image quality | Increase brightness sliders in dashboard. Ensure RPM is stable (check Serial Monitor). |
| Python sender "No such file" error | `cd` into the `API/` folder before running the script. |
| Python sender can't connect to fan | Confirm fan IP in Serial Monitor. Both PC and fan must be on same network. |
| Clock/Text not showing on fan | Click a Send button in the Clock & Text panel — it uploads as a new animation. |
| OTA update fails | Try via web dashboard `/update` page instead of Arduino IDE. |

---

## Notes

- The `.ino` firmware is **unchanged** from the original — only `webpage.h` and the Python sender UI labels have been updated to **Fusion 5**
- All media is stored as polar-mapped JPEG frames in LittleFS
- Maximum stream buffer size is 32KB per JPEG frame
- Double buffering ensures smooth frame transitions during rotation
- The hall sensor debounce is set to 5ms, supporting high RPM operation

---

*Fusion 5 POV Hologram Fan · Built on ESP32-S3*
