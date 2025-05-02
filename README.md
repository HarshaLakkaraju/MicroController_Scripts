# MicroController Code Hub 🚀  
*A collection projects for IoT development*
---
## **⚡ Quick Setup Guide (Arduino IDE)**

### 1. **Install Arduino IDE**  
Download from official source:  
[arduino_software](https://www.arduino.cc/en/software)

### 2. **Add ESP Board Support**  
1. Open Arduino IDE: **File → Preferences**  
2. In **Additional Boards Manager URLs**, paste:  
  ```markdown
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   https://dl.espressif.com/dl/package_esp32_index.json
  ```
3. Click **OK** to save

### 3. **Install Board Packages**  
1. **Tools → Board → Boards Manager**  
2. Search and install:  
   - `ESP8266` by ESP8266 Community  
   - `ESP32` by Espressif Systems  

### 4. **Install Required Libraries**  
1. **Sketch → Include Library → Manage Libraries**  
2. Search and install:  
   - `ESP8266WiFi` (ESP8266 WiFi support)  
   - `WiFiClientSecure` (ESP32 HTTPS)  
   - `ESP8266WebServer`/`WebServer` (HTTP server)  
   - `EEPROM` (Storage operations)  
---
## **🔧 Troubleshooting**  

### **Board Not Detected?**  
- Install USB drivers:  
  - CH340: [https://www.wch.cn/downloads/CH341SER_EXE.html](https://www.wch.cn/downloads/CH341SER_EXE.html)  
  - CP2102: [https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)  
- Verify **Tools → Port** selection  

### **Compilation Errors?**  
- Confirm board selection:  
  - ESP8266: `NodeMCU 1.0 (ESP-12E Module)`  
  - ESP32: `ESP32 Dev Module`  
- Update packages: **Boards Manager → Update**  

---

## **📜 License**  
MIT Licensed - Open for modification and distribution  

---

**🚀 Happy IoT Developing!**  
*Let's build the future, one microcontroller at a time!*
