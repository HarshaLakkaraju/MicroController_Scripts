

# ESP32 PS5 Power Controller

> Remotely power on your PS5 from anywhere using an ESP32 and servo motor via secure connection.This will **simulate a physical hardware power-button press**, allowing the PS5 to start like normal button press.


## 📋 Overview

This project allows you to physically press your PS5 power button remotely using an ESP32 microcontroller with a servo motor. The system is designed to work through your home network with asecure remote access, work over LAN.

* **Non-Blocking:** Acknowledges HTTP requests *before* moving the servo to prevent client timeouts.
* **Safety-First:** Automatically detaches the servo to prevent jitter, heating, and brownouts.
* **Self-Healing:** Automatically reconnects to WiFi if the router reboots or signal is lost.

---

## 🎯 Features

- ✅ **Remote Power Control** - Turn on PS5 from anywhere,Trigger via HTTP `GET` request
- ✅ **WiFi Auto-Reconnect** - Automatically reconnects if WiFi drops
- ✅ **Web Interface** - Modern dark-themed control panel
- ✅ **Real-time Status** - Live monitoring of device state
- ✅ **Non-blocking Operation** - Responsive even during button press
- ✅ **LED Status Indicators** - Visual feedback for all states
- ✅ **Secure Architecture** - LAN-only ESP, no internet exposure
- ✅ **Production Ready** - Stable 24/7 operation
- ✅ **JSON API** - `/status` endpoint for integration with Home Assistant or Python dashboards.
- ✅ **Visual Feedback** - LED codes for WiFi status and action confirmation.

## 🛠️ Hardware Requirements

### Required Components
- **ESP32** development board (any variant)
- **SG90 Servo Motor** (or compatible 5V servo)
- **Jumper Wires** (3 wires minimum)
- **USB Cable** for ESP32 programming
- **5V Power Supply** (ESP32 USB power is sufficient for SG90)

### Optional Components
- External 5V power supply (if servo causes brownouts)
- Mounting bracket/3D printed holder for servo
- Case for ESP32

## 📐 Wiring Diagram

```
ESP32          SG90 Servo
─────          ──────────
GPIO 18  ────► Signal (Orange/Yellow)
5V       ────► VCC (Red)
GND      ────► GND (Brown/Black)

```


### Pin Configuration
| Component | Pin | Notes |
|-----------|-----|-------|
| Servo Signal | GPIO 18 | PWM capable pin |
| Built-in LED | GPIO 2 | Status indicator |
| Servo Power | 5V | From ESP32 |
| Servo Ground | GND | Common ground |

## 🚀 Installation

### 1. Arduino IDE Setup

1. Install **Arduino IDE** (v2.0+ recommended)
2. Add ESP32 board support:
   - Go to `File` → `Preferences`
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
3. Install ESP32 boards:
   - `Tools` → `Board` → `Boards Manager`
   - Search "ESP32" and install

### 2. Install Required Libraries

Install these libraries via `Tools` → `Manage Libraries`:

- **ESP32Servo** by Kevin Harrington (v0.13.0+)
- WiFi (built-in with ESP32 core)

### 3. Configure the Code

Edit the configuration section in the code:

```cpp
// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";      // Your WiFi network name
const char* password = "YOUR_WIFI_PASS";  // Your WiFi password

// Optional: Adjust servo angles if needed
const int restAngle = 0;      // Arm away from button
const int pressAngle = 90;    // Arm presses button (adjust 80-110)
const int pressDuration = 600; // Press hold time in milliseconds
```

### 4. Upload to ESP32

1. Connect ESP32 via USB
2. Select board: `Tools` → `Board` → `ESP32 Dev Module`
3. Select port: `Tools` → `Port` → (your ESP32 port)
4. Click **Upload**
5. Open `Serial Monitor` (115200 baud) to see connection status

## 🌐 Usage

### Local Access (Same Network)

Once uploaded and connected, the ESP32 will display its IP address in the Serial Monitor:

```
✓ Access at: http://192.168.1.XXX
```

#### Web Interface
Open the displayed IP in your browser to access the control panel.

#### Direct API Calls
```bash
# Press PS5 power button
curl http://192.168.1.XXX/press

# Check status
curl http://192.168.1.XXX/status

# Web interface
curl http://192.168.1.XXX/
```


## 📡 API Endpoints

### `GET /press`
Triggers the physical button press sequence.

**Response Codes:**
- `200 OK` - Button press started
- `503 Service Unavailable` - Already pressing or WiFi down

**Example:**
```bash
curl http://192.168.1.50/press
# Response: Button press started
```

### `GET /status`
Returns current device status in JSON format.

**Response:**
```json
{
  "status": "online",
  "ip": "192.168.1.50",
  "pressing": false,
  "wifi": "YourNetworkName",
  "rssi": -45,
  "uptime": 123456
}
```

### `GET /`
Loads the web-based control interface with live status updates.

## 🔧 Configuration Options

### Servo Angle Adjustment

If the servo doesn't press the button correctly:

```cpp
// Too weak? Increase press angle
const int pressAngle = 100;  // Try 95-110

// Too strong? Decrease press angle  
const int pressAngle = 85;   // Try 80-90

// Adjust press duration if needed
const int pressDuration = 800; // Try 400-1000ms
```

### Static IP Configuration

For stable remote access, configure static IP in `connectToWiFi()`:

```cpp
void connectToWiFi() {
  // Add before WiFi.begin()
  IPAddress local_IP(192, 168, 1, 50);     // Your chosen IP
  IPAddress gateway(192, 168, 1, 1);       // Your router IP
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.config(local_IP, gateway, subnet);
  
  // Continue with existing code...
}
```

## 🔒 Security Architecture

```
Internet
    ↓ (Remote VPN)
Home Server (Ubuntu)
    ↓ (Local HTTP)
ESP32 (LAN Only)
    ↓ (PWM Signal)
Servo → PS5 Button
```

**Security Features:**
- ESP32 never exposed to internet
- LAN-only HTTP communication
- Twingate handles authentication
- No port forwarding required
- No cloud dependencies

## 🐛 Troubleshooting

### ESP32 Won't Connect to WiFi

**Symptoms:** Serial shows "Failed to connect!"

**Solutions:**
1. Verify WiFi credentials (case-sensitive)
2. Check WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
3. Move ESP32 closer to router
4. Check router allows new device connections

### Servo Doesn't Move

**Symptoms:** No physical movement when pressing button

**Solutions:**
1. Verify wiring (especially signal to GPIO 18)
2. Check servo power connection (red to 5V)
3. Test servo with manual code:
   ```cpp
   buttonServo.attach(18);
   buttonServo.write(90);
   delay(1000);
   buttonServo.write(0);
   ```
4. Try external 5V power supply if brownouts occur

### ESP32 Resets When Servo Moves

**Symptoms:** ESP32 reboots during button press

**Solutions:**
1. Use external 5V power supply for servo
2. Keep ESP32 and servo grounds connected
3. Add 100µF capacitor across servo power/ground
4. Reduce `pressAngle` to limit servo current draw

### Can't Access Web Interface

**Symptoms:** Browser can't reach ESP32

**Solutions:**
1. Check Serial Monitor for correct IP address
2. Ensure device is on same WiFi network
3. Try `ping` ESP32 IP from computer
4. Disable firewall temporarily for testing
5. Set DHCP reservation in router

### Button Press Doesn't Turn On PS5

**Symptoms:** Servo moves but PS5 doesn't respond

**Solutions:**
1. Adjust `pressAngle` (try 95-110°)
2. Increase `pressDuration` (try 800-1000ms)
3. Verify servo arm physically contacts button
4. Check button press is firm enough
5. Ensure PS5 is in rest mode (not fully off)

## 📊 LED Status Indicators

| LED State | Meaning |
|-----------|---------|
| **Blinking** | Connecting to WiFi |
| **Solid On** | Connected and ready |
| **Rapid Flash** | Button press in progress |
| **Blinking Slow** | WiFi disconnected, attempting reconnect |
| **Off** | Not powered or failed to connect |

## 🔄 Integration with Home Server

To integrate with Python server for remote Twingate access:

### Python Server Example
```python
from flask import Flask
import requests

app = Flask(__name__)
ESP_IP = "192.168.1.50"

@app.route('/power-on')
def power_on():
    try:
        response = requests.get(f"http://{ESP_IP}/press", timeout=3)
        return response.text, response.status_code
    except:
        return "ESP Unreachable", 503

app.run(host='0.0.0.0', port=8080)
```

## 📝 System Requirements

### ESP32
- **Flash:** 4MB minimum
- **RAM:** 520KB (standard ESP32)
- **WiFi:** 2.4GHz 802.11 b/g/n

### Network
- **Router:** 2.4GHz WiFi support
- **DHCP:** Enabled (or configure static IP)
- **Firewall:** Allow LAN communication

### Power
- **ESP32:** 160-260mA (via USB)
- **SG90 Servo:** 100-250mA (during movement)
- **Total:** ~500mA peak (standard USB port sufficient)

## 🤝 Contributing

Improvements welcome! Areas for contribution:
- Multi-device support (PC, Xbox, etc.)
- MQTT integration
- Home Assistant addon
- Mobile app
- 3D printable mounting brackets

## 📄 License

This project is open source and available for personal use.

## ⚠️ Disclaimer

This project involves physical interaction with electronic devices. Use at your own risk. Ensure proper mounting to avoid damage to your PS5 or other equipment.

## 🔗 Related Projects

- [Python Server for Twingate Integration](#) (coming soon)
- [Home Assistant Integration](#) (coming soon)
- [3D Printable Servo Mount](#) (coming soon)

## 📞 Support

For issues or questions:
1. Check troubleshooting section above
2. Review Serial Monitor output
3. Open an issue with:
   - ESP32 model
   - Serial Monitor output
   - Network configuration
   - Photos of wiring (if hardware issue)

---

**Version:** 1.2  
**Last Updated:** January 2026  
**Tested With:** ESP32-WROOM-32, SG90 Servo, PS5 Slim/Standard