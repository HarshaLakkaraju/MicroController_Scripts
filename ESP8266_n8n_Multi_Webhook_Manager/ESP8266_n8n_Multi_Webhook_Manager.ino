/*
  Project: ESP8266 Multi Webhook Manager
  Description:
    A lightweight web interface hosted on an ESP8266 microcontroller to manage up to 3 webhooks.
    Allows adding, updating, triggering, and deleting webhook URLs via a secure login interface.
    Data is stored in EEPROM, and communication is handled using the ESP8266WebServer.

  Features:
    - User authentication (simple, insecure - for demo purposes only)
    - Persistent storage of webhook data via EEPROM
    - Responsive HTML interface with JavaScript
    - Webhook triggering via HTTP GET
    - EEPROM read/write with JSON serialization

  Author: [Your Name or GitHub Handle]
  License: MIT
*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char* ssid        = " wifi name ";
const char* password    = "wifi password ";

// Simple login credentials (for demonstration only)
const char* adminUser   = "usename";
const char* adminPass   = "password";

// EEPROM and Webhook settings
#define EEPROM_SIZE 512
#define MAX_WEBHOOKS 3

struct Webhook {
  String name;
  String url;
};

Webhook webhooks[MAX_WEBHOOKS];
ESP8266WebServer server(80);

// Track login state with a simple flag
bool isLoggedIn = false;

// --------------------------------------------------------------------
// EEPROM & JSON Helpers
// --------------------------------------------------------------------
void saveWebhooksToEEPROM() {
  StaticJsonDocument<256> doc;
  JsonArray arr = doc.createNestedArray("webhooks");

  for (int i = 0; i < MAX_WEBHOOKS; i++) {
    if (webhooks[i].name.length() && webhooks[i].url.length()) {
      JsonObject wObj = arr.createNestedObject();
      wObj["name"] = webhooks[i].name;
      wObj["url"]  = webhooks[i].url;
    }
  }
  String output;
  serializeJson(doc, output);

  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, (i < (int)output.length()) ? output[i] : 0);
  }
  EEPROM.commit();
}

void loadWebhooksFromEEPROM() {
  char buffer[EEPROM_SIZE + 1];
  for (int i = 0; i < EEPROM_SIZE; i++) {
    buffer[i] = EEPROM.read(i);
  }
  buffer[EEPROM_SIZE] = '\0';

  // Clear the array first
  for (int i = 0; i < MAX_WEBHOOKS; i++) {
    webhooks[i].name = "";
    webhooks[i].url  = "";
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, buffer);
  if (!error) {
    JsonArray arr = doc["webhooks"].as<JsonArray>();
    int idx = 0;
    for (JsonObject obj : arr) {
      if (idx >= MAX_WEBHOOKS) break;
      webhooks[idx].name = obj["name"].as<String>();
      webhooks[idx].url  = obj["url"].as<String>();
      idx++;
    }
  }
}

// --------------------------------------------------------------------
// Trigger & Delete
// --------------------------------------------------------------------
// Return the numeric HTTP status code from the remote server, or a negative value on failure
int sendWebhook(int index) {
  if (index < 0 || index >= MAX_WEBHOOKS) return -1; 
  if (!webhooks[index].url.length()) return -2; 

  WiFiClient client;
  HTTPClient http;
  http.begin(client, webhooks[index].url);

  int httpCode = http.GET();
  http.end();
  return httpCode; // Return the actual code (e.g., 200, 404, etc.)
}

bool deleteWebhook(int index) {
  if (index < 0 || index >= MAX_WEBHOOKS) return false;
  webhooks[index].name = "";
  webhooks[index].url  = "";
  return true;
}

// --------------------------------------------------------------------
// Pages
// --------------------------------------------------------------------
String loginPage() {
  return String(F("<!DOCTYPE html><html><head>"
                  "<meta charset='utf-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<title>Login</title>"
                  "<style>"
                  "body{font-family:Arial,Helvetica,sans-serif;margin:0;padding:0;background:#eef1f7;}"
                  ".login-container{max-width:400px;margin:60px auto;padding:20px;background:#fff;"
                  "border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);text-align:center;}"
                  "h1{margin-bottom:20px;color:#333;}"
                  "label{display:block;text-align:left;margin:10px 0 5px;font-weight:bold;}"
                  "input{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ccc;border-radius:4px;"
                  "box-sizing:border-box;font-size:1rem;}"
                  "button{padding:10px 20px;border:none;border-radius:4px;background:#3b82f6;color:#fff;"
                  "font-weight:bold;cursor:pointer;}"
                  "button:hover{background:#2563eb;}"
                  "</style>"
                  "</head><body>"
                  "<div class='login-container'>"
                  "<h1>Login</h1>"
                  "<form action='/login' method='POST'>"
                  "<label>Username</label>"
                  "<input type='text' name='user' required>"
                  "<label>Password</label>"
                  "<input type='password' name='pass' required>"
                  "<button type='submit'>Login</button>"
                  "</form>"
                  "</div></body></html>"));
}

String mainPage() {
  String page = F("<!DOCTYPE html><html lang='en'><head>"
                  "<meta charset='UTF-8'/>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'/>"
                  "<title>ESP8266 - Multi Webhooks</title>"
                  "<style>"
                  "body{font-family:Arial,Helvetica,sans-serif;margin:0;padding:0;background:#eef1f7;}"
                  "header{background:#4b5563;color:#fff;padding:20px;text-align:center;}"
                  "h1{margin:0;font-size:1.8rem;}"
                  ".container{max-width:700px;margin:20px auto;padding:20px;background:#fff;"
                  "border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);}"
                  "#responseMessage{background:#f6f8fa;padding:10px;margin-bottom:15px;border-radius:6px;"
                  "font-weight:bold;color:#333;display:none;}"
                  ".section-title{margin-top:0;margin-bottom:20px;font-size:1.4rem;color:#333;}"
                  "#webhook-list .item{background:#f9fafc;padding:15px;margin-bottom:12px;"
                  "border-radius:8px;display:flex;justify-content:space-between;align-items:center;}"
                  ".item-details{max-width:70%;line-height:1.4;}"
                  ".btn{padding:8px 12px;border-radius:5px;border:none;cursor:pointer;"
                  "font-weight:bold;margin-left:5px;}"
                  ".trigger-btn{background:#3b82f6;color:#fff;}"
                  ".trigger-btn:hover{background:#2563eb;}"
                  ".delete-btn{background:#f87171;color:#fff;}"
                  ".delete-btn:hover{background:#ef4444;}"
                  ".form-section{margin-top:30px;}"
                  ".form-group{margin-bottom:15px;}"
                  ".form-group label{display:block;font-weight:bold;margin-bottom:5px;font-size:0.9rem;}"
                  ".form-group input{width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;"
                  "font-size:0.9rem;box-sizing:border-box;}"
                  ".save-btn{margin-top:8px;padding:10px 20px;background:#10b981;color:#fff;border:none;"
                  "border-radius:5px;font-weight:bold;cursor:pointer;transition:background 0.2s;}"
                  ".save-btn:hover{background:#059669;}"
                  "</style>"
                  "</head><body>"
                  "<header><h1>Multi Webhook Manager</h1></header>"
                  "<div class='container'>"
                  "<div id='responseMessage'></div>"
                  "<h2 class='section-title'>Stored Webhooks</h2>"
                  "<div id='webhook-list'></div>"
                  "<div class='form-section'>"
                  "<h2 class='section-title'>Add / Update Webhook</h2>"
                  "<div class='form-group'>"
                  "<label>Index (0-");
  page += String(MAX_WEBHOOKS - 1);
  page += F(")</label>"
            "<input type='number' id='idxInput' min='0' max='");
  page += String(MAX_WEBHOOKS - 1);
  page += F("' required>"
            "</div>"
            "<div class='form-group'>"
            "<label>Webhook Name</label>"
            "<input type='text' id='nameInput' required>"
            "</div>"
            "<div class='form-group'>"
            "<label>Webhook URL</label>"
            "<input type='url' id='urlInput' required>"
            "</div>"
            "<button class='save-btn' onclick='saveWebhook()'>Save Webhook</button>"
            "</div>"
            "</div>"
            "<script>"
            "function showMessage(text){"
            "  let msgEl=document.getElementById('responseMessage');"
            "  msgEl.style.display='block';"
            "  msgEl.textContent=text||'';"
            "  setTimeout(()=>{msgEl.style.display='none';},5000);"
            "}"
            "async function loadWebhooks(){"
            "  let res = await fetch('/list');"
            "  if(res.status===200){"
            "    let data = await res.json();"
            "    let container = document.getElementById('webhook-list');"
            "    container.innerHTML='';"
            "    data.webhooks.forEach((item,idx)=>{"
            "      let div = document.createElement('div');"
            "      div.className='item';"
            "      if(item.name && item.url){"
            "        div.innerHTML = `<div class='item-details'><strong>Index: </strong>${idx}<br>"
            "                        <strong>Name: </strong>${item.name}<br>"
            "                        <strong>URL: </strong>${item.url}</div>"
            "                        <div><button class='btn trigger-btn' onclick='triggerWebhook(${idx})'>Trigger</button>"
            "                        <button class='btn delete-btn' onclick='deleteWebhook(${idx})'>Delete</button></div>`;"
            "      } else {"
            "        div.innerHTML=`<div class='item-details'>Index ${idx} is empty</div>`;"
            "      }"
            "      container.appendChild(div);"
            "    });"
            "  } else {"
            "    showMessage('Error: Please login again.');"
            "    window.location='/';"
            "  }"
            "}"
            "async function triggerWebhook(i){"
            "  let res = await fetch(`/trigger?idx=${i}`, {method:'POST'});"
            "  let msg = await res.text();"
            "  if(res.status===200){"
            "    showMessage('Success: '+msg);"
            "  } else if(res.status===400){"
            "    showMessage('Bad request: '+msg);"
            "  } else {"
            "    showMessage('Error '+res.status+': '+msg);"
            "  }"
            "}"
            "async function deleteWebhook(i){"
            "  let confirmed = confirm('Are you sure you want to delete this webhook?');"
            "  if(!confirmed)return;"
            "  let res = await fetch(`/delete?idx=${i}`, {method:'POST'});"
            "  let msg = await res.text();"
            "  if(res.status===200){"
            "    showMessage('Success: '+msg);"
            "  } else if(res.status===400){"
            "    showMessage('Bad request: '+msg);"
            "  } else {"
            "    showMessage('Error '+res.status+': '+msg);"
            "  }"
            "  loadWebhooks();"
            "}"
            "async function saveWebhook(){"
            "  let idxVal=document.getElementById('idxInput').value;"
            "  let nameVal=document.getElementById('nameInput').value;"
            "  let urlVal=document.getElementById('urlInput').value;"
            "  if(!idxVal||!nameVal||!urlVal){"
            "    showMessage('Please fill all fields');"
            "    return;"
            "  }"
            "  let params=new URLSearchParams();"
            "  params.append('idx',idxVal);"
            "  params.append('name',nameVal);"
            "  params.append('url',urlVal);"
            "  let res=await fetch('/save',{method:'POST',body:params});"
            "  let msg=await res.text();"
            "  if(res.status===200){"
            "    showMessage('Success: '+msg);"
            "  } else if(res.status===400){"
            "    showMessage('Bad request: '+msg);"
            "  } else {"
            "    showMessage('Error '+res.status+': '+msg);"
            "  }"
            "  loadWebhooks();"
            "}"
            "window.onload=loadWebhooks;"
            "</script>"
            "</body></html>");
  return page;
}

// --------------------------------------------------------------------
// Handlers
// --------------------------------------------------------------------
void handleLoginPage() {
  // If already logged in, show main page
  if (isLoggedIn) {
    server.send(200, "text/html", mainPage());
  } else {
    // Show styled login page
    server.send(200, "text/html", loginPage());
  }
}

void handleLogin() {
  String user = server.arg("user");
  String pass = server.arg("pass");

  if (user == adminUser && pass == adminPass) {
    isLoggedIn = true;
    server.sendHeader("Location", "/");
    server.send(302);
  } else {
    server.send(200, "text/html",
                "<h2>Login Failed</h2><p><a href='/'>Try again</a></p>");
  }
}

// Return JSON list of webhooks
void handleList() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(302);
    return;
  }
  StaticJsonDocument<256> doc;
  JsonArray arr = doc.createNestedArray("webhooks");
  for (int i = 0; i < MAX_WEBHOOKS; i++) {
    JsonObject wObj = arr.createNestedObject();
    wObj["name"] = webhooks[i].name;
    wObj["url"]  = webhooks[i].url;
  }
  String result;
  serializeJson(doc, result);
  server.send(200, "application/json", result);
}

// Save or Update a webhook
void handleSave() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(302);
    return;
  }

  if (!(server.hasArg("idx") && server.hasArg("name") && server.hasArg("url"))) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= MAX_WEBHOOKS) {
    server.send(400, "text/plain", "Index out of range");
    return;
  }
  webhooks[idx].name = server.arg("name");
  webhooks[idx].url  = server.arg("url");
  saveWebhooksToEEPROM();
  server.send(200, "text/plain", "Saved");
}

// Trigger a webhook
void handleTrigger() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(302);
    return;
  }

  if (!server.hasArg("idx")) {
    server.send(400, "text/plain", "Missing index");
    return;
  }
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= MAX_WEBHOOKS) {
    server.send(400, "text/plain", "Index out of range");
    return;
  }
  int httpCode = sendWebhook(idx);  // returns actual code from the GET
  if (httpCode == 200) {
    server.send(200, "text/plain", "Triggered Successfully (200 OK)");
  } else if (httpCode > 0) {
    // Some other HTTP status code
    String message = "Triggered but server responded with code: " + String(httpCode);
    server.send(200, "text/plain", message);
  } else {
    // Negative or zero means a failure in the request
    String errorMsg = "Failed or invalid code: " + String(httpCode);
    server.send(400, "text/plain", errorMsg);
  }
}

// Delete a webhook
void handleDelete() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(302);
    return;
  }

  if (!server.hasArg("idx")) {
    server.send(400, "text/plain", "Missing index");
    return;
  }
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= MAX_WEBHOOKS) {
    server.send(400, "text/plain", "Index out of range");
    return;
  }
  bool result = deleteWebhook(idx);
  saveWebhooksToEEPROM();
  if (result) {
    server.send(200, "text/plain", "Deleted");
  } else {
    server.send(400, "text/plain", "Failed");
  }
}

// --------------------------------------------------------------------
// Setup & Main Loop
// --------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadWebhooksFromEEPROM();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  server.on("/",         HTTP_GET,  handleLoginPage);
  server.on("/login",    HTTP_POST, handleLogin);
  server.on("/list",     HTTP_GET,  handleList);
  server.on("/save",     HTTP_POST, handleSave);
  server.on("/trigger",  HTTP_POST, handleTrigger);
  server.on("/delete",   HTTP_POST, handleDelete);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
