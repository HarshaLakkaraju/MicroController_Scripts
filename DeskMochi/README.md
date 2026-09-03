# 🍡 Desk Mochi — Intelligent Stand-Up Reminder

A minimal, animated desk companion built on an ESP32 and a 128×64 SSD1306 OLED display.
Desk Mochi sits on your desk, counts down your sitting time, and reminds you to stand up using two expressive, Looi-inspired eyes.

It runs as a local HTTP server on your Wi-Fi network. Any device, script, or automation tool (like **n8n**) can control it by sending simple web requests — no cloud, no account, no subscription.

> **Animation philosophy:** Two solid white rounded rectangles. No pupils, no mouth, no icons.
> Every single emotion — from joy to suspicion to a full glitch — is expressed through eye **shape**, **size**, and **position** only.
> Inspired by the [Looi robot](https://looirobot.com/).

---

## 📋 Table of Contents

1. [Hardware](#1-hardware)
2. [Software Dependencies](#2-software-dependencies)
3. [Arduino IDE Setup & Upload](#3-arduino-ide-setup--upload)
4. [Configuration](#4-configuration-configh)
5. [Software Architecture](#5-software-architecture)
6. [All 17 Expressions](#6-all-17-expressions)
7. [HTTP API Reference](#7-http-api-reference)
8. [Home Server Automation](#8-home-server-automation)
9. [n8n Workflow Blueprint](#9-n8n-workflow-blueprint)
10. [Troubleshooting Log](#10-troubleshooting-log)

---

## 1. Hardware

| Component | Detail | Pin |
|-----------|--------|-----|
| **ESP32 Dev Board** | Classic ESP32 (NOT C3 or S3) | — |
| **SSD1306 OLED** | 128×64, I2C | — |
| OLED `SDA` | — | GPIO **21** |
| OLED `SCL` | — | GPIO **22** |
| OLED `VCC` | — | **3V3** |
| OLED `GND` | — | **GND** |

No other hardware required. No buttons, buzzer, or battery.

---

## 2. Software Dependencies

### Install via Arduino Library Manager
**Sketch → Include Library → Manage Libraries**

| Library | Author |
|---------|--------|
| Adafruit SSD1306 | Adafruit |
| Adafruit GFX Library | Adafruit |

> When asked "Install missing dependencies?" click **Install All** — this pulls in `Adafruit BusIO`.

### Install via ZIP (required — not in standard Library Manager)

Download both files:
- 📦 [AsyncTCP-master.zip](https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip)
- 📦 [ESPAsyncWebServer-master.zip](https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip)

Then: **Sketch → Include Library → Add .ZIP Library...**
Add `AsyncTCP` first, then `ESPAsyncWebServer`.

### Built-in (no install needed)
`WiFi.h` · `Wire.h` · `ESPmDNS.h`

---

## 3. Arduino IDE Setup & Upload

### Step 1 — Add ESP32 board support
**Preferences → Additional Boards Manager URLs:**
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
**Tools → Board → Boards Manager** → search `esp32` → install **Espressif Systems**.

### Step 2 — Set Wi-Fi credentials
Open the `config.h` tab inside Arduino IDE and edit:
```c
#define WIFI_SSID     "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"
```

### Step 3 — Select board and port
- **Tools → Board → esp32 → ESP32 Dev Module**
- **Tools → Port** → select your COM or `/dev/cu.*` port

### Step 4 — Upload
Click the **Upload →** button.

> ⚠️ **If upload hangs at "Connecting..."**
> Hold the **BOOT** button on the ESP32 while "Connecting..." is shown. Release when the percentage counter starts.

### Step 5 — Find the IP address
Open **Serial Monitor** at **115200 baud**. Press **EN/RST** on the ESP32.
The IP address will be printed in the Serial Monitor and displayed on the OLED screen.

---

## 4. Configuration (`config.h`)

### Wi-Fi & Network
| Constant | Default | Description |
|----------|---------|-------------|
| `WIFI_SSID` | `"YOUR_SSID_HERE"` | Your Wi-Fi network name |
| `WIFI_PASSWORD` | `"YOUR_PASSWORD_HERE"` | Your Wi-Fi password |
| `MDNS_HOSTNAME` | `"mochi"` | Makes device reachable at `mochi.local` |
| `DEFAULT_SIT_MINUTES` | `45` | Countdown duration when no `minutes` param is sent |

### OLED Hardware
| Constant | Default | Description |
|----------|---------|-------------|
| `OLED_ADDR` | `0x3C` | I2C address — change to `0x3D` if screen stays blank |
| `OLED_SDA` | `21` | I2C data pin |
| `OLED_SCL` | `22` | I2C clock pin |

### Animation Timing
| Constant | Default | Description |
|----------|---------|-------------|
| `BLINK_INTERVAL_MIN_MS` | `2000` | Min time between blinks |
| `BLINK_INTERVAL_MAX_MS` | `5000` | Max time between blinks |
| `BLINK_CLOSE_MS` | `120` | How long eyes stay closed per blink |
| `BREATH_INTERVAL_MS` | `4500` | Breathing bob period (SITTING only) |
| `MICRO_MOTION_INTERVAL_MS` | `1200` | Whole-eye drift tick |
| `TIMESUP_FLASH_MS` | `500` | TIME'S UP flash half-period (2 Hz) |
| `NOTIFY_DURATION_MS` | `4000` | How long NOTIFY overlay stays visible |
| `NOTIFY_ENTRANCE_MS` | `300` | Eye pop-in animation duration |
| `FRAME_INTERVAL_MS` | `33` | ~30 fps frame period |

---

## 5. Software Architecture

Desk Mochi is **single-threaded and non-blocking**. The `loop()` function never calls `delay()` after setup, so the web server responds to HTTP requests instantly even while animations run at 30 fps.

### The 4-State Machine
```
  [boot]
    │
    ▼
  IDLE ──[POST /pomodoro/start]──► SITTING ──[countdown = 0]──► TIMESUP
    ▲                                 │                               │
    └─────────[POST /pomodoro/stop]───┴───────────────────────────────┘

  NOTIFY (4 s overlay) can be triggered from ANY state.
  Underlying timers keep running during NOTIFY.
```

### The Core Drawing Primitive: `drawLooiEye()`
All 17 expressions are built from calling this one function twice (left eye + right eye):

```
drawLooiEye(cx, cy, ew, openH, style)
```

| Style | Name | Visual |
|-------|------|--------|
| `0` | Normal | All 4 corners rounded |
| `1` | Flat Top | Straight top edge — focused/squinting |
| `2` | Happy Arc | Only upper arc visible `∩` — joyful |

### Animation Layers (all independent, millis-based)
| Layer | Period | Effect |
|-------|--------|--------|
| Micro-motion | 1.2 s | Whole eye body drifts ±1 px |
| Blink | Random 2–5 s | Eye height collapses to 2 px line |
| Breathing | 4.5 s | Eyes bob ±1 px vertically (SITTING only) |
| Flash | 500 ms | Entire frame toggles on/off (TIMESUP only) |
| Pop-in | 300 ms one-shot | Eyes scale 0→1 on NOTIFY entry |

---

## 6. All 17 Expressions

### Core State Expressions (always running)

| # | State | Expression | Description |
|---|-------|------------|-------------|
| 1 | IDLE | **Relaxed** | Large round eyes, random blink, subtle drift |
| 2 | SITTING | **Focused** | Flat-top squinted eyes + MM:SS countdown |
| 3 | TIMESUP | **Shocked** | Max size eyes flashing at 2 Hz until stopped |

### Notification Expressions (`POST /notify type=<name>`)

| # | Type | Expression | Description |
|---|------|------------|-------------|
| 4 | `email` | **Joyful** | Happy arc `∩∩`, pop-in entrance |
| 5 | `chat` | **Delighted** | Widest, thinnest happy arc |
| 6 | `alert` | **Alarmed** | Large normal eyes, static stare |
| 7 | `generic` | **Curious** | Asymmetric — right eye higher+wider (head tilt) |

### Fun / Ambient Animations (`POST /notify type=<name>`)

| # | Type | Expression | Description |
|---|------|------------|-------------|
| 8 | `fly` | **Tracking a fly** | Both eyes snap to random positions every 150–350 ms. Occasional squint when "locking on." |
| 9 | `cycling` | **Cycling/Jogging** | Left and right eyes bob up/down in opposite phase at 600 ms. Eyes lean slightly left. |
| 10 | `pingpong` | **Ping-Pong** | Eyes snap hard left (400 ms) → centre (100 ms) → right (400 ms) → repeat. |
| 11 | `sleepy` | **Nodding Off** | Eyes droop flat over 2.5 s, then snap wide open in a startled wake. Loops. |
| 12 | `suspicious` | **Scanning** | Thin flat-top slits slowly pan left→right→left over 2 s. |
| 13 | `dizzy` | **Dizzy** | Both eye bodies trace a small ellipse (800 ms revolution). |
| 14 | `shy` | **Peeking** | Happy-arc eyes sink off-screen, peek up for 1 s, then drop back down. |
| 15 | `groove` | **Dancing** | Eyes bounce at 150 BPM with squash-and-stretch physics. |
| 16 | `thinking` | **Thinking** | Left eye expands while right shrinks, then swaps. Smooth 1.2 s sine cycle. |
| 17 | `glitch` | **Glitch** | Rapid position jitter every 30–150 ms. 20% chance one eye tears to a 2 px flat line. |

All NOTIFY animations play for exactly **4 seconds** then automatically return to the previous state. The sitting countdown keeps running underneath.

---

## 7. HTTP API Reference

Mochi listens on **port 80**. Reach it at `http://mochi.local` or by IP address.

### `GET /`
Lists all available routes and current mode.

### `GET /status`
```bash
curl http://mochi.local/status
# → {"mode":"SITTING","seconds_left":2340,"notify_type":null}
```

### `POST /pomodoro/start`
```bash
curl -X POST http://mochi.local/pomodoro/start -d "minutes=45"
curl -X POST http://mochi.local/pomodoro/start           # uses default
```

### `POST /pomodoro/stop`
Cancels countdown or clears the TIMESUP flash. Always returns to IDLE.
```bash
curl -X POST http://mochi.local/pomodoro/stop
```

### `POST /notify`
```bash
# Notification expressions
curl -X POST http://mochi.local/notify -d "type=email"
curl -X POST http://mochi.local/notify -d "type=chat"
curl -X POST http://mochi.local/notify -d "type=alert"
curl -X POST http://mochi.local/notify -d "type=generic"

# Fun ambient animations
curl -X POST http://mochi.local/notify -d "type=fly"
curl -X POST http://mochi.local/notify -d "type=cycling"
curl -X POST http://mochi.local/notify -d "type=pingpong"
curl -X POST http://mochi.local/notify -d "type=sleepy"
curl -X POST http://mochi.local/notify -d "type=suspicious"
curl -X POST http://mochi.local/notify -d "type=dizzy"
curl -X POST http://mochi.local/notify -d "type=shy"
curl -X POST http://mochi.local/notify -d "type=groove"
curl -X POST http://mochi.local/notify -d "type=thinking"
curl -X POST http://mochi.local/notify -d "type=glitch"
```

---

## 8. Home Server Automation

The most powerful setup is to run a lightweight Python script on your home server that periodically checks Mochi's status and sends ambient animations automatically while you work.

### `mochi_ambient.py`

```python
import urllib.request
import urllib.parse
import json
import time
import random

# --- CONFIGURATION ---
MOCHI_URL = "http://mochi.local"   # or use IP: "http://192.168.1.55"
NORMAL_INTERVAL = 300              # 5 minutes
ERROR_INTERVAL = 3600              # 1 hour (when Mochi is offline)

ANIMATIONS = ["fly", "suspicious", "dizzy", "shy", "thinking"]
# ---------------------

print("🍡 Mochi Ambient Service started. Running in background...")

while True:
    try:
        response = urllib.request.urlopen(f"{MOCHI_URL}/status", timeout=5)

        if response.getcode() != 200:
            raise Exception(f"HTTP Error: {response.getcode()}")

        data = json.loads(response.read().decode())
        current_mode = data.get("mode", "UNKNOWN")

        if current_mode == "SITTING":
            chosen_anim = random.choice(ANIMATIONS)
            print(f"Mochi is SITTING. Triggering: {chosen_anim}")
            post_data = urllib.parse.urlencode({"type": chosen_anim}).encode()
            urllib.request.urlopen(f"{MOCHI_URL}/notify", data=post_data, timeout=5)
        else:
            print(f"Mochi is {current_mode}. Skipping animation.")

        time.sleep(NORMAL_INTERVAL)

    except Exception as e:
        print(f"Connection failed ({e}). Mochi might be offline.")
        print("Pausing for 1 hour before retrying...")
        time.sleep(ERROR_INTERVAL)
```

> **No pip install needed.** Uses only Python's standard library.

---

### Running as a systemd Service (Linux)

This keeps the script running forever — even after server reboots.

**Step 1:** Create the service file:
```bash
sudo nano /etc/systemd/system/mochi.service
```

**Step 2:** Paste this (replace `your_username` with your actual Linux username):
```ini
[Unit]
Description=Mochi Ambient Animation Service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=your_username
ExecStart=/usr/bin/python3 /home/your_username/mochi_ambient.py
Restart=on-failure
RestartSec=30
StartLimitIntervalSec=600
StartLimitBurst=5
StandardOutput=journal
StandardError=journal
LogRateLimitIntervalSec=0
MemoryMax=64M
CPUQuota=10%

[Install]
WantedBy=multi-user.target
```

**Step 3:** Cap journal size to protect disk space:
```bash
sudo nano /etc/systemd/journald.conf
```
Add this line under `[Journal]`:
```ini
SystemMaxUse=50M
```

**Step 4:** Apply everything:
```bash
sudo systemctl restart systemd-journald
sudo systemctl daemon-reload
sudo systemctl enable mochi.service
sudo systemctl start mochi.service
```

### Managing the Service

```bash
# Check status
systemctl status mochi.service

# Watch live logs
journalctl -u mochi.service -f

# See last 20 lines
journalctl -u mochi.service -n 20

# Restart after config change
sudo systemctl restart mochi.service

# Check disk used by logs
journalctl --disk-usage

# Confirm auto-start is enabled
systemctl is-enabled mochi.service
```

---

## 9. n8n Workflow Blueprint

Since Mochi is controlled entirely by HTTP calls, n8n (or any automation tool) is the ideal "brain."

### Workflow 1: Auto-Presence Timer
Trigger `POST /pomodoro/start` when you unlock your PC (Windows: Task Scheduler on Event ID 4801).
Trigger `POST /pomodoro/stop` when you lock it (Event ID 4800).
Result: Zero-touch — Mochi tracks your actual screen time automatically.

### Workflow 2: AI Notification Filter
1. n8n catches incoming messages (Slack, Email, Discord)
2. Pass the message text to a local AI (e.g. Hermes, OpenClaw) with the prompt:
   > *"Is this message URGENT, CELEBRATION, or CASUAL? Reply with one word only."*
3. Route the output to the matching Mochi expression:
   - `URGENT` → `type=alert`
   - `CELEBRATION` → `type=groove`
   - `CASUAL` → `type=chat`

### Workflow 3: Stand-Up Enforcer
n8n polls `GET /status` every 1 minute.
If `mode == "TIMESUP"`:
- Flash Philips Hue lights
- Send push notification to your phone
- When you stand up, trigger `POST /pomodoro/stop`

### Workflow 4: Ambient Life (Covered by `mochi_ambient.py` above)
Every 5–15 minutes, poll status. If `SITTING`, send a random fun animation.

---

## 10. Troubleshooting Log

This section documents every issue encountered during setup.

---

### ❌ `ESPAsyncWebServer.h: No such file or directory`
**Cause:** The library is not in the standard Library Manager and must be installed manually.

**Fix:** Download both ZIPs from GitHub and install via **Sketch → Include Library → Add .ZIP Library:**
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip)

---

### ❌ `no matching function for call to 'max(int, int16_t)'`
**Cause:** C++ template deduction fails when `max()` receives two arguments of different types (`int` literal vs `int16_t`).

**Fix:** Cast all integer literals to `(int16_t)`:
```cpp
// ❌ Before
int16_t pw = max(4, (int16_t)(ew * pf));

// ✅ After
int16_t pw = max((int16_t)4, (int16_t)(ew * pf));
```

---

### ❌ Upload hangs at "Connecting..."
**Cause:** The ESP32 is not in flashing mode.

**Fix:** Hold the **BOOT** button on the ESP32 while the Arduino IDE shows "Connecting...". Release it as soon as the upload percentage counter starts.

---

### ❌ OLED screen stays completely blank after boot
**Cause:** The I2C address of your specific OLED module is `0x3D` instead of the default `0x3C`.

**Fix:** In `config.h`, change:
```c
#define OLED_ADDR 0x3C   // change to 0x3D
```

---

### ❌ `mochi.local` not found from the Linux server (Name or service not known)
**Cause:** `.local` mDNS resolution requires the `avahi-daemon` package, which is not installed by default on all Linux server distros. Without it, the server cannot resolve `mochi.local` even though it works fine from Mac/Windows.

**Fix — Option A (Recommended): Install avahi**
```bash
sudo apt-get update && sudo apt-get install -y avahi-daemon
sudo systemctl restart mochi.service
```

**Fix — Option B: Use the raw IP address**
Find Mochi's IP from the OLED screen or your router's device list, then edit `mochi_ambient.py`:
```python
# Change this:
MOCHI_URL = "http://mochi.local"

# To this:
MOCHI_URL = "http://192.168.1.55"   # ← your Mochi's actual IP
```
Then restart: `sudo systemctl restart mochi.service`

---

### ❌ `mochi.local` not found on Windows
**Cause:** Windows does not have a built-in mDNS resolver unless Bonjour is installed.

**Fix:** Install [Bonjour for Windows](https://support.apple.com/downloads/bonjour-for-windows), or use the raw IP address instead.

---

### ❌ Journal logs filling disk over long-term use
**Cause:** `StandardOutput=journal` writes every `print()` to disk permanently if persistent journald logging is enabled.

**Fix:** Cap journal size in `/etc/systemd/journald.conf`:
```ini
[Journal]
SystemMaxUse=50M
```
Apply: `sudo systemctl restart systemd-journald`

---

### ❌ Service respawns rapidly on startup crash (restart storm)
**Cause:** If the script crashes immediately (bad path, wrong Python version), `Restart=on-failure` + `RestartSec=10` can create a tight restart loop.

**Fix:** Use explicit limits in the service file:
```ini
RestartSec=30
StartLimitIntervalSec=600
StartLimitBurst=5
```
This allows max 5 restarts within 10 minutes before systemd marks the service as `failed` and stops retrying.

---

## File Structure

```
DeskMochi/
├── DeskMochi.ino       ← Main firmware (Arduino IDE)
├── config.h            ← Wi-Fi credentials and all tunable constants
├── mochi_ambient.py    ← Home server ambient animation script
└── README.md           ← This file
```

---

## License

MIT — do whatever you want with it.
