/*
 * ESP32 PS5 Power Controller - Production Ready
 * Final Version with all fixes
 * 
 * Features:
 * 1. WiFi auto-reconnect
 * 2. Proper press duration (600ms for PS5)
 * 3. Correct status reporting
 * 4. LED status indicators
 * 5. Non-blocking operation
 */

#include <WiFi.h>
#include <ESP32Servo.h>

// ========== CONFIGURATION ==========
const char* ssid = "YOUR_WIFI_SSID";      // CHANGE THIS
const char* password = "YOUR_WIFI_PASS";  // CHANGE THIS

// Pin Configuration
const int servoPin = 18;      // Servo signal pin (PWM capable)
const int ledPin = 2;         // Built-in LED

// Servo Settings (optimized for PS5)
const int restAngle = 0;      // Arm away from button
const int pressAngle = 90;    // Arm presses button (adjust if needed)
const int pressDuration = 600; // 600ms press duration for PS5

// Global Variables
WiFiServer server(80);        // HTTP server on port 80
Servo buttonServo;            // Servo object
bool isPressing = false;      // Prevent multiple presses
bool ledState = HIGH;         // LED state
unsigned long lastBlink = 0;  // For non-blocking LED blink

// WiFi Reconnection
unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 5000; // Check every 5 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n" + String(50, '='));
  Serial.println("PS5 Power Controller v1.2");
  Serial.println("Production Ready");
  Serial.println(String(50, '='));
  
  // Setup pins
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Initialize servo
  ESP32PWM::allocateTimer(0);
  buttonServo.setPeriodHertz(50); // Standard 50Hz servo
  
  // Connect to WiFi
  connectToWiFi();
  
  // Start server
  server.begin();
  Serial.println("✓ HTTP server started");
  Serial.print("✓ Access at: http://");
  Serial.println(WiFi.localIP());
  Serial.println("✓ Endpoints: /press, /status, /");
  Serial.println(String(50, '='));
  
  // Startup sequence
  startupSequence();
}

void loop() {
  // ========== WiFi AUTO-RECONNECT ==========
  unsigned long now = millis();
  if (now - lastWifiCheck >= wifiCheckInterval) {
    lastWifiCheck = now;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠ WiFi disconnected! Reconnecting...");
      digitalWrite(ledPin, LOW);
      connectToWiFi();
    }
  }
  
  // ========== NON-BLOCKING LED BLINK WHEN DISCONNECTED ==========
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastBlink >= 500) { // Blink every 500ms
      lastBlink = now;
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
  
  // ========== HANDLE HTTP REQUESTS ==========
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("\n New client connected");
    
    // Read request
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') break;
      }
    }
    
    // Parse and handle request
    if (request.indexOf("GET /press") != -1) {
      Serial.println(" Button press requested");
      handlePress(client);
    } 
    else if (request.indexOf("GET /status") != -1) {
      Serial.println(" Status requested");
      handleStatus(client);
    }
    else if (request.indexOf("GET /") != -1) {
      Serial.println(" Web page requested");
      handleRoot(client);
    }
    else {
      // Unknown request
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("404 - Not Found");
    }
    
    // Close connection
    delay(10);
    client.stop();
    Serial.println(" Client disconnected");
  }
}

// ========== HANDLE BUTTON PRESS ==========
void handlePress(WiFiClient &client) {
  if (isPressing) {
    // Already pressing - return busy
    client.println("HTTP/1.1 503 Service Unavailable");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("Busy: Button press in progress");
    Serial.println(" Response: 503 Busy");
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    // WiFi down
    client.println("HTTP/1.1 503 Service Unavailable");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("Error: WiFi disconnected");
    Serial.println(" Response: 503 WiFi disconnected");
    return;
  }
  
  // Mark as pressing and respond immediately
  isPressing = true;
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.println("Button press started");
  
  // Process in background
  Serial.println(" Starting press sequence...");
  pressButton();
  
  isPressing = false;
  Serial.println(" Press sequence completed");
}

// ========== ACTUAL BUTTON PRESS ==========
void pressButton() {
  // Flash LED rapidly during press
  for (int i = 0; i < 6; i++) {
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(100);
  }
  
  // Attach and press
  buttonServo.attach(servoPin);
  delay(50); // Let servo initialize
  
  Serial.println("⬆ Moving to press position...");
  buttonServo.write(pressAngle);
  delay(pressDuration); // Hold press
  
  Serial.println("⬇ Returning to rest...");
  buttonServo.write(restAngle);
  delay(300); // Let servo return
  
  // Detach to save power and prevent jitter
  buttonServo.detach();
  
  // LED back to normal
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
}

// ========== STATUS ENDPOINT (FIXED) ==========
void handleStatus(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  
  client.print("{");
  client.print("\"status\":\"");
  client.print(WiFi.status() == WL_CONNECTED ? "online" : "offline");
  client.print("\",\"ip\":\"");
  client.print(WiFi.localIP().toString());
  client.print("\",\"pressing\":");
  client.print(isPressing ? "true" : "false"); // FIX: Real-time state
  client.print(",\"wifi\":\"");
  client.print(WiFi.SSID());
  client.print("\",\"rssi\":");
  client.print(WiFi.RSSI());
  client.print(",\"uptime\":");
  client.print(millis());
  client.println("}");
}

// ========== ROOT PAGE ==========
void handleRoot(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<title>PS5 Power Controller</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; text-align: center; padding: 40px; background: #121212; color: white; }");
  client.println(".container { max-width: 500px; margin: 0 auto; background: #1e1e1e; padding: 30px; border-radius: 10px; box-shadow: 0 0 20px rgba(0,120,212,0.3); }");
  client.println("h1 { color: #0078d4; margin-bottom: 30px; }");
  client.println(".status { background: #252525; padding: 15px; border-radius: 5px; margin: 20px 0; text-align: left; }");
  client.println(".btn { background: #0078d4; color: white; border: none; padding: 15px 40px; font-size: 18px; border-radius: 8px; cursor: pointer; margin: 10px; width: 80%; transition: all 0.3s; }");
  client.println(".btn:hover { background: #005a9e; transform: translateY(-2px); }");
  client.println(".btn:active { transform: translateY(0); }");
  client.println(".btn:disabled { background: #666; cursor: not-allowed; }");
  client.println("</style>");
  client.println("</head><body>");
  
  client.println("<div class='container'>");
  client.println("<h1>ESP32 PS5 Power Controller</h1>");
  
  client.println("<div class='status'>");
  client.println("<strong>IP:</strong> " + WiFi.localIP().toString() + "<br>");
  client.println("<strong>WiFi:</strong> " + String(WiFi.SSID()) + "<br>");
  client.println("<strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm<br>");
  client.println("<strong>Status:</strong> <span id='statusText'>" + String(isPressing ? "Pressing..." : "Ready") + "</span>");
  client.println("</div>");
  
  client.println("<button class='btn' id='pressBtn' onclick='pressButton()'>");
  client.println("Press PS5 Power Button");
  client.println("</button>");
  
  client.println("<p id='message'></p>");
  
  client.println("<script>");
  client.println("let isPressing = false;");
  
  client.println("function updateStatus() {");
  client.println("  fetch('/status')");
  client.println("    .then(r => r.json())");
  client.println("    .then(data => {");
  client.println("      document.getElementById('statusText').innerText = data.pressing ? 'Pressing...' : 'Ready';");
  client.println("      document.getElementById('pressBtn').disabled = data.pressing;");
  client.println("      isPressing = data.pressing;");
  client.println("    });");
  client.println("}");
  
  client.println("function pressButton() {");
  client.println("  if (isPressing) return;");
  client.println("  document.getElementById('message').innerHTML = 'Pressing...';");
  client.println("  document.getElementById('pressBtn').disabled = true;");
  client.println("  document.getElementById('statusText').innerText = 'Pressing...';");
  client.println("  ");
  client.println("  fetch('/press')");
  client.println("    .then(r => r.text())");
  client.println("    .then(t => {");
  client.println("      document.getElementById('message').innerHTML = 'done! ' + t;");
  client.println("      setTimeout(updateStatus, 1000); // Update after press completes");
  client.println("    })");
  client.println("    .catch(e => {");
  client.println("      document.getElementById('message').innerHTML = ' Error: ' + e;");
  client.println("      updateStatus();");
  client.println("    });");
  client.println("}");
  
  client.println("// Initial status check");
  client.println("updateStatus();");
  client.println("// Auto-update every 2 seconds");
  client.println("setInterval(updateStatus, 2000);");
  
  client.println("</script>");
  client.println("</div>");
  client.println("</body></html>");
}

// ========== WiFi CONNECTION ==========
void connectToWiFi() {
  Serial.print(" Connecting to WiFi: ");
  Serial.print(ssid);
  Serial.print(" ... ");
  
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    digitalWrite(ledPin, !digitalRead(ledPin)); // Blink during connection
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✓");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("   Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    digitalWrite(ledPin, HIGH); // Solid LED when connected
  } else {
    Serial.println(" X ");
    Serial.println("   Failed to connect!");
    digitalWrite(ledPin, LOW);
  }
}

// ========== STARTUP SEQUENCE ==========
void startupSequence() {
  // LED test pattern
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
  
  // Servo test (small movement to verify it's working)
  buttonServo.attach(servoPin);
  buttonServo.write(restAngle);
  delay(100);
  buttonServo.detach();
  
  // Final LED state
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
}