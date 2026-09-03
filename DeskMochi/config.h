#pragma once

// ──────────────────────────────────────────────────────────────────────────────
//  Desk Mochi — WiFi credentials
//  Edit WIFI_SSID and WIFI_PASSWORD before flashing.
// ──────────────────────────────────────────────────────────────────────────────
#define WIFI_SSID     "YOUR_SSID_HERE"
#define WIFI_PASSWORD "YOUR_PASSWORD_HERE"

// Default sit-time if /pomodoro/start receives no 'minutes' param (minutes)
#define DEFAULT_SIT_MINUTES 45

// mDNS hostname — device will be reachable at mochi.local
#define MDNS_HOSTNAME "mochi"

// ── OLED ──────────────────────────────────────────────────────────────────────
#define OLED_WIDTH   128
#define OLED_HEIGHT   64
#define OLED_ADDR   0x3C   // most common; try 0x3D if display stays blank
#define OLED_SDA      21
#define OLED_SCL      22
#define OLED_RESET    -1   // share Arduino reset

// ── Timing constants (milliseconds) ──────────────────────────────────────────
#define BLINK_INTERVAL_MIN_MS   2000   // fastest auto-blink period
#define BLINK_INTERVAL_MAX_MS   5000   // slowest auto-blink period
#define BLINK_CLOSE_MS           120   // eye-closed duration
#define BREATH_INTERVAL_MS      4500   // sit-state breathing cycle
#define MICRO_MOTION_INTERVAL_MS 1200  // global alive-motion tick
#define TIMESUP_FLASH_MS         500   // TIME'S UP on/off half-period (2 Hz)
#define NOTIFY_DURATION_MS      4000   // overlay lifetime
#define NOTIFY_ENTRANCE_MS       300   // eye pop-in animation duration

// ── Display refresh ───────────────────────────────────────────────────────────
#define FRAME_INTERVAL_MS        33    // ~30 fps
