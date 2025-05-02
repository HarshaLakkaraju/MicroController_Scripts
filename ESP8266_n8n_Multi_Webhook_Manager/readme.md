# ESP8266 Multi Webhooks Manager

This project provides a simple web interface for managing multiple webhook URLs on an ESP8266 device. You can update, delete, or trigger webhooks directly from a web UI after logging in with basic credentials.

## Features

- Stores up to three webhooks (configurable in the code).
- Allows adding/editing webhooks: Each webhook has a name and URL.
- Supports “Trigger” and “Delete” operations for each webhook.
- Password-protected login to prevent unauthorized access.
- Saves all webhook data in the EEPROM (persists after resets).

## How to Change Wi-Fi Credentials

Inside the code, look for the WiFi credentials section:
```cpp
// Wi-Fi credentials
const char* ssid        = "<<your wi-fi name>>";
const char* password    = "<<password>>";
```
Replace "loading...." and "fuckyou@" with your personal Wi-Fi SSID and password.

## How to Change Login Credentials

Inside the code, locate the variables for login authentication:
```cpp
// Simple login credentials (for demonstration only)
const char* adminUser   = "<<website username>>";
const char* adminPass   = "<<login passwiord>>";
```
Change "admin" and "admin123" to any username and password of your choice.

## Web Interface Preview

Below are placeholders for interface screenshots, which you can replace with real images:

![Login Page](https://github.com/HarshaLakkaraju/MicroController_Scripts/blob/main/ESP8266_n8n_Multi_Webhook_Manager/data/login%20page%20(1).png)

![Main Webhook Dashboard
(https://github.com/HarshaLakkaraju/MicroController_Scripts/blob/main/ESP8266_n8n_Multi_Webhook_Manager/data/main%20page.png)

Use these screenshots or your own to visualize how to navigate through the web UI.

## What is this sketch about ?
This example consists of a web page that displays misc ESP8266 information, namely values of GPIOs, ADC measurement and free heap 
using http requests and a html/javascript frontend.
A similar functionality used to be hidden in previous versions of the FSBrowser example.

## How to use it ?
1. Uncomment one of the `#define USE_xxx` directives in the sketch to select the ESP filesystem to store the index.htm file on
2. Provide the credentials of your WiFi network (search for `STASSID`)
3. Compile and upload the sketch to your ESP8266 device
4. For normal use, copy the contents of the `data` folder to the filesystem. To do so:
- for SDFS, copy that contents (not the data folder itself, just its contents) to the root of a FAT/FAT32-formated SD card connected to the SPI port of the ESP8266
- for SPIFFS or LittleFS, please follow the instructions at https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system
5. Once the data and sketch have been uploaded, access the page by pointing your browser to http://graph.local

## What does it demonstrate ?
1. Use of the ESP8266WebServer library to return files from the ESP filesystem
2. Use of the ESP8266WebServer library to return a dynamic JSON document
3. Querying the state of ESP I/Os as well as free heap
4. Ajax calls to a JSON API of the ESP from a webpage
5. Rendering of "real-time" data in a webpage


