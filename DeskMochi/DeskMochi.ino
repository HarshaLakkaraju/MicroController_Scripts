// ─────────────────────────────────────────────────────────────────────────────
//  Desk Mochi — Stand-up Reminder Companion
//  ESP32 + SSD1306 128×64 OLED
//  Animation style: Looi-inspired — two large rounded-rectangle eyes ONLY.
//  No mouth, no cheeks, no icons on the face. Pure eyes.
// ─────────────────────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Globals — display & server
// ─────────────────────────────────────────────────────────────────────────────
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
AsyncWebServer   server(80);

// ─────────────────────────────────────────────────────────────────────────────
//  State machine
// ─────────────────────────────────────────────────────────────────────────────
enum AppState { STATE_IDLE, STATE_SITTING, STATE_TIMESUP, STATE_NOTIFY };

volatile AppState appState    = STATE_IDLE;
volatile AppState underState  = STATE_IDLE;   // state beneath NOTIFY overlay

// SITTING countdown
volatile uint32_t sitStartMs    = 0;
volatile uint32_t sitDurationMs = DEFAULT_SIT_MINUTES * 60UL * 1000UL;

// NOTIFY overlay
volatile uint32_t notifyStartMs = 0;
String            notifyType    = "generic";

// ─────────────────────────────────────────────────────────────────────────────
//  Animation state  (all millis-based, zero delay())
// ─────────────────────────────────────────────────────────────────────────────

// ── Blink ─────────────────────────────────────────────────────────────────────
uint32_t nextBlinkMs  = 0;
bool     eyesClosed   = false;
uint32_t blinkCloseMs = 0;

// ── Micro-motion — whole eye body drifts ±1 px for an alive feel ─────────────
uint32_t nextMicroMs = 0;
int8_t   microDx     = 0;
int8_t   microDy     = 0;

// ── Breathing (SITTING) ───────────────────────────────────────────────────────
uint32_t nextBreathMs  = 0;
int8_t   breathOffsetY = 0;

// ── TIME'S UP flash ───────────────────────────────────────────────────────────
uint32_t lastFlashMs  = 0;
bool     flashVisible = true;

// ── NOTIFY entrance: eyes scale 0→1 over 300 ms ──────────────────────────────
bool     notifyEntering   = true;
uint32_t notifyEnterStart = 0;

// ── Fly animation — eyes dart to random positions every ~200 ms ───────────────
uint32_t flyNextJumpMs = 0;
int8_t   flyEyeX = 0, flyEyeY = 0;
bool     flySquint = false;

// ── Glitch animation — rapid random jitter + occasional eye tear ──────────────
uint32_t glitchNextMs  = 0;
int8_t   glitchX = 0, glitchY = 0;
bool     glitchTear    = false;
uint8_t  glitchTearEye = 0;   // 0 = left eye tears, 1 = right eye tears

// ── Frame rate limiter ────────────────────────────────────────────────────────
uint32_t lastFrameMs = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t randomRange(uint32_t lo, uint32_t hi) {
    return lo + (esp_random() % (hi - lo + 1));
}
static void scheduleNextBlink() {
    nextBlinkMs = millis() + randomRange(BLINK_INTERVAL_MIN_MS, BLINK_INTERVAL_MAX_MS);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Core Looi-style eye primitive — CLEAN, no pupil
//
//  cx, cy  – centre of the eye body (already includes micro-drift offset)
//  ew      – eye width  (px)
//  openH   – eye height (px) — pass small value for blink, 0 collapses to line
//  style:
//    0 = NORMAL     — all four corners rounded
//    1 = FLAT_TOP   — top edge straight (focused / squinting look)
//    2 = HAPPY_ARC  — only upper arc visible (happy crescent ∩)
// ─────────────────────────────────────────────────────────────────────────────
static void drawLooiEye(int16_t cx, int16_t cy, int16_t ew, int16_t openH, uint8_t style) {
    if (openH < 2) openH = 2;
    if (ew    < 4) ew    = 4;

    // Corner radius: proportional, capped so small eyes still look round
    int16_t r = min(ew, openH) / 5;
    if (r < 2) r = 2;
    if (r > 9) r = 9;

    int16_t ex = cx - ew    / 2;
    int16_t ey = cy - openH / 2;

    if (style == 2) {
        // HAPPY_ARC: draw a full double-height rounded rect,
        // then mask the bottom half black → leaves a clean upper arc ∩
        int16_t fullH = openH * 2;
        display.fillRoundRect(ex, ey, ew, fullH, r, WHITE);
        display.fillRect(ex, ey + openH, ew, fullH, BLACK);
    } else {
        display.fillRoundRect(ex, ey, ew, openH, r, WHITE);
        if (style == 1) {
            // FLAT_TOP: fill the top corner gaps → straight horizontal top edge
            display.fillRect(ex, ey, ew, r, WHITE);
        }
    }
    // No pupil — solid clean white shape only.
}

// ─────────────────────────────────────────────────────────────────────────────
//  Layout constants
//  Screen 128×64.  Two 38 px wide eyes with 14 px gap → 19 px side margins.
//  Left eye centre X=38, Right eye centre X=90, Normal Y=25.
// ─────────────────────────────────────────────────────────────────────────────
static const int16_t LEX = 38;
static const int16_t REX = 90;
static const int16_t NEY = 25;

// ─────────────────────────────────────────────────────────────────────────────
//  7 Face Expressions  (no text, no pupil — shape + motion only)
// ─────────────────────────────────────────────────────────────────────────────

// ── 1. IDLE ───────────────────────────────────────────────────────────────────
//  Large, fully rounded eyes. Whole eye body drifts ±1 px with micro-motion.
//  Blinks every 2–5 s (height collapses to a 2 px line, then reopens).
static void drawFaceIdle(float openFrac, int8_t dx, int8_t dy) {
    int16_t h = (int16_t)(30 * openFrac);
    if (h < 2) h = 2;
    drawLooiEye(LEX + dx, NEY + dy, 38, h, 0);
    drawLooiEye(REX + dx, NEY + dy, 38, h, 0);
}

// ── 2. SITTING (countdown) ────────────────────────────────────────────────────
//  Flat-top squinted eyes (focused look). Both eyes bob ±1 px with breathing.
//  Countdown MM:SS shown below — the only text anywhere in the firmware.
static void drawFaceSitting(float openFrac, int8_t dx, int8_t dy,
                            int8_t breathY, uint32_t secsLeft) {
    int16_t ey = 20 + breathY + dy;
    int16_t h  = (int16_t)(20 * openFrac);
    if (h < 2) h = 2;
    drawLooiEye(LEX + dx, ey, 38, h, 1);   // style 1 = flat top
    drawLooiEye(REX + dx, ey, 38, h, 1);

    char buf[8];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", secsLeft / 60, secsLeft % 60);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((OLED_WIDTH - (int16_t)strlen(buf) * 12) / 2, 44);
    display.print(buf);
}

// ── 3. TIME'S UP (flashing) ───────────────────────────────────────────────────
//  Largest eyes, full 2 Hz on/off flash — no text, the flash communicates urgency.
static void drawFaceTimesUp(bool visible) {
    if (!visible) return;
    drawLooiEye(LEX, 22, 46, 36, 0);
    drawLooiEye(REX, 22, 46, 36, 0);
}

// ── 4. NOTIFY — EMAIL ─────────────────────────────────────────────────────────
//  Happy arc ∩ eyes, medium width. Pop-in scale on entry.
static void drawFaceEmail(float sc, int8_t dx, int8_t dy) {
    int16_t ew = (int16_t)(40 * sc);
    int16_t h  = (int16_t)(18 * sc);
    if (ew < 4) ew = 4;
    if (h  < 2) h  = 2;
    drawLooiEye(LEX + dx, NEY + dy, ew, h, 2);
    drawLooiEye(REX + dx, NEY + dy, ew, h, 2);
}

// ── 5. NOTIFY — CHAT ──────────────────────────────────────────────────────────
//  Even slimmer happy arc — widest & thinnest, maximum squint of delight.
static void drawFaceChat(float sc, int8_t dx, int8_t dy) {
    int16_t ew = (int16_t)(44 * sc);
    int16_t h  = (int16_t)(13 * sc);
    if (ew < 4) ew = 4;
    if (h  < 2) h  = 2;
    drawLooiEye(LEX + dx, NEY + 2 + dy, ew, h, 2);
    drawLooiEye(REX + dx, NEY + 2 + dy, ew, h, 2);
}

// ── 6. NOTIFY — ALERT ─────────────────────────────────────────────────────────
//  Very large normal eyes — static alarmed stare (no flash here).
static void drawFaceAlert(float sc, int8_t dx, int8_t dy) {
    int16_t ew = (int16_t)(44 * sc);
    int16_t h  = (int16_t)(36 * sc);
    if (ew < 4) ew = 4;
    if (h  < 2) h  = 2;
    drawLooiEye(LEX + dx, 22 + dy, ew, h, 0);
    drawLooiEye(REX + dx, 22 + dy, ew, h, 0);
}

// ── 7. NOTIFY — GENERIC (curious) ────────────────────────────────────────────
//  Asymmetric: right eye 4 px wider and 5 px higher than left → head-tilt look.
static void drawFaceGeneric(float sc, int8_t dx, int8_t dy) {
    int16_t ewL = (int16_t)(36 * sc),  hL = (int16_t)(28 * sc);
    int16_t ewR = (int16_t)(40 * sc),  hR = (int16_t)(28 * sc);
    if (ewL < 4) ewL = 4; if (hL < 2) hL = 2;
    if (ewR < 4) ewR = 4; if (hR < 2) hR = 2;
    drawLooiEye(LEX + dx, NEY + 2 + dy, ewL, hL, 0);   // left: lower
    drawLooiEye(REX + dx, NEY - 3 + dy, ewR, hR, 0);   // right: raised
}

// ─────────────────────────────────────────────────────────────────────────────
//  EXTRA ANIMATIONS (all triggered via POST /notify type=<name>)
//  All built with the same drawLooiEye primitive — no style changes.
// ─────────────────────────────────────────────────────────────────────────────

// ── 8. FLY  ───────────────────────────────────────────────────────────────────
//  Eyes dart around erratically tracking an invisible fly.
//  Every 150–350 ms: snap to a new random X/Y. Occasional squint when "focusing".
//  Both eyes always move together so they share the same target.
static void drawFaceFly() {
    uint32_t now = millis();
    if (now >= flyNextJumpMs) {
        flyNextJumpMs = now + randomRange(150, 350);
        flyEyeX  = (int8_t)((int32_t)(esp_random() % 27) - 13);  // ±13 px
        flyEyeY  = (int8_t)((int32_t)(esp_random() % 21) - 8);   // -8…+12 px
        flySquint = (esp_random() % 5 == 0);                       // 20% chance squint
    }
    int16_t h = flySquint ? 12 : 28;
    uint8_t s = flySquint ? 1 : 0;
    // Clamp so eyes never leave the screen
    int16_t lx = constrain((int16_t)(LEX + flyEyeX), 20, 56);
    int16_t rx = constrain((int16_t)(REX + flyEyeX), 72, 108);
    int16_t ey = constrain((int16_t)(NEY + flyEyeY), 10, 44);
    drawLooiEye(lx, ey, 36, h, s);
    drawLooiEye(rx, ey, 36, h, s);
}

// ── 9. CYCLING / JOGGING ──────────────────────────────────────────────────────
//  Left and right eyes bob up/down in opposite phase — simulates leg pumping.
//  Eyes also lean slightly left (forward lean during effort).
static void drawFaceCycling(uint32_t t) {
    float phase = (float)(t % 600) / 600.0f * 6.2832f;   // 0→2π per 600 ms
    int8_t leftY  = (int8_t)( sinf(phase)        * 6.0f);
    int8_t rightY = (int8_t)(-sinf(phase)        * 6.0f);  // opposite phase
    drawLooiEye(LEX - 2, NEY + leftY,  36, 26, 0);          // slight left lean
    drawLooiEye(REX - 2, NEY + rightY, 36, 26, 0);
}

// ── 10. PING-PONG ─────────────────────────────────────────────────────────────
//  Eyes snap hard left, pause, snap hard right, pause — watching a rally.
//  Pattern: left 400 ms → centre 100 ms → right 400 ms → centre 100 ms → …
static void drawFacePingPong(uint32_t t) {
    uint32_t cycle = t % 1000;
    int8_t dx;
    if      (cycle < 400)  dx = -16;   // ball on the left
    else if (cycle < 500)  dx =   0;   // ball passing centre
    else if (cycle < 900)  dx =  16;   // ball on the right
    else                   dx =   0;   // ball passing centre again
    drawLooiEye(LEX + dx, NEY, 34, 26, 0);
    drawLooiEye(REX + dx, NEY, 34, 26, 0);
}

// ── 11. SLEEPY ────────────────────────────────────────────────────────────────
//  Eyes slowly droop (flat-top as they narrow), then snap open with a start.
//  Cycle: 2.5 s droop → instant snap to fully open → 0.5 s hold → repeat.
static void drawFaceSleepy(uint32_t t) {
    uint32_t cycle = t % 3000;
    float openF;
    if (cycle < 2500) {
        openF = 1.0f - (float)cycle / 2500.0f;
        if (openF < 0.07f) openF = 0.07f;   // never fully shut; teases the blink
    } else {
        openF = 1.0f;   // snap wide open!
    }
    int16_t h = (int16_t)(28.0f * openF);
    if (h < 2) h = 2;
    uint8_t s = (openF < 0.65f) ? 1 : 0;   // flat-top starts once half-closed
    drawLooiEye(LEX, NEY, 36, h, s);
    drawLooiEye(REX, NEY, 36, h, s);
}

// ── 12. SUSPICIOUS ────────────────────────────────────────────────────────────
//  Flat-top slits that slowly pan from left to right and back — scanning.
static void drawFaceSuspicious(uint32_t t) {
    float phase = (float)(t % 2000) / 2000.0f;   // 0→1 per 2 s
    // Triangle wave: 0→1 in first half, 1→0 in second half
    float pos = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
    int8_t dx = (int8_t)((pos - 0.5f) * 28.0f);  // ±14 px sweep
    drawLooiEye(LEX + dx, NEY, 36, 7, 1);          // style 1 = flat-top slits
    drawLooiEye(REX + dx, NEY, 36, 7, 1);
}

// ── 13. DIZZY ─────────────────────────────────────────────────────────────────
//  Both eye bodies trace a small ellipse together — googly-eye rotation.
//  800 ms per full revolution.
static void drawFaceDizzy(uint32_t t) {
    float angle = (float)(t % 800) / 800.0f * 6.2832f;
    int8_t cx = (int8_t)( cosf(angle) * 8.0f);
    int8_t cy = (int8_t)( sinf(angle) * 5.0f);
    drawLooiEye(LEX + cx, NEY + cy, 34, 26, 0);
    drawLooiEye(REX + cx, NEY + cy, 34, 26, 0);
}

// ── 14. SHY ───────────────────────────────────────────────────────────────────
//  Happy-arc eyes sink below the screen edge and slowly peek back up.
//  Cycle: slide up to peek (0.5 s) → hold peek (1 s) → drop back (1 s) → wait.
static void drawFaceShy(uint32_t t) {
    uint32_t cycle = t % 3000;
    int8_t offsetY;
    if (cycle < 500) {
        // Rise: 30 → 12 px below normal position
        float p = (float)cycle / 500.0f;
        offsetY = (int8_t)(30.0f - p * 18.0f);
    } else if (cycle < 1500) {
        offsetY = 12;   // Hold peek
    } else {
        // Drop back down quickly
        float p = (float)(cycle - 1500) / 1000.0f;
        offsetY = (int8_t)(12.0f + p * 18.0f);
        if (offsetY > 30) offsetY = 30;
    }
    // Happy-arc while peeking — looks cuter than raw rectangles here
    drawLooiEye(LEX, NEY + offsetY, 36, 16, 2);
    drawLooiEye(REX, NEY + offsetY, 36, 16, 2);
}

// ── 15. GROOVE ────────────────────────────────────────────────────────────────
//  Eyes bounce to a beat (150 BPM = 400 ms/beat).
//  Squash & stretch: wider + shorter at the bottom of the bounce.
static void drawFaceGroove(uint32_t t) {
    float beat = (float)(t % 400) / 400.0f;
    // Sharp attack (0→0.3), slow release (0.3→1.0)
    float bounce = (beat < 0.3f) ? (beat / 0.3f) : (1.0f - (beat - 0.3f) / 0.7f);
    int8_t  dy = (int8_t)(bounce * 8.0f);
    int16_t ew = 36 + (int16_t)(bounce * 7.0f);   // squash wider at bottom
    int16_t h  = 28 - (int16_t)(bounce * 7.0f);   // squash shorter at bottom
    if (ew > 46) ew = 46;
    if (h  < 4)  h  = 4;
    drawLooiEye(LEX, NEY + dy, ew, h, 0);
    drawLooiEye(REX, NEY + dy, ew, h, 0);
}

// ── 16. THINKING ──────────────────────────────────────────────────────────────
//  Left and right eyes pulse inversely — one expands while the other shrinks.
//  1.2 s per full cycle, smooth sine curve.
static void drawFaceThinking(uint32_t t) {
    float sine = sinf((float)(t % 1200) / 1200.0f * 6.2832f);  // -1…+1
    int16_t ewL = (int16_t)(36 + sine * 9.0f);
    int16_t hL  = (int16_t)(26 + sine * 9.0f);
    int16_t ewR = (int16_t)(36 - sine * 9.0f);
    int16_t hR  = (int16_t)(26 - sine * 9.0f);
    if (ewL < 8) ewL = 8; if (hL < 4) hL = 4;
    if (ewR < 8) ewR = 8; if (hR < 4) hR = 4;
    drawLooiEye(LEX, NEY, ewL, hL, 0);
    drawLooiEye(REX, NEY, ewR, hR, 0);
}

// ── 17. GLITCH ────────────────────────────────────────────────────────────────
//  Rapid X/Y jitter every 30–150 ms, occasional "tear" where one eye collapses
//  to a 2 px line while the other stays wide open — CRT glitch aesthetic.
static void drawFaceGlitch() {
    uint32_t now = millis();
    if (now >= glitchNextMs) {
        glitchNextMs  = now + randomRange(30, 150);
        glitchX       = (int8_t)((int32_t)(esp_random() % 11) - 5);
        glitchY       = (int8_t)((int32_t)(esp_random() %  9) - 4);
        glitchTear    = (esp_random() % 5 == 0);    // 20% chance of a tear
        glitchTearEye = (uint8_t)(esp_random() % 2);
    }
    int16_t hL = 28, hR = 28;
    if (glitchTear) {
        if (glitchTearEye == 0) hL = 2; else hR = 2;
    }
    drawLooiEye(LEX + glitchX, NEY + glitchY, 36, hL, 0);
    drawLooiEye(REX + glitchX, NEY + glitchY, 36, hR, 0);
}


// ─────────────────────────────────────────────────────────────────────────────
static void renderFrame() {
    uint32_t now = millis();
    AppState cur = appState;
    display.clearDisplay();

    // ── Blink ─────────────────────────────────────────────────────────────────
    bool blinkOn = (cur == STATE_IDLE || cur == STATE_SITTING);
    if (blinkOn && !eyesClosed && (int32_t)(now - nextBlinkMs) >= 0) {
        eyesClosed   = true;
        blinkCloseMs = now;
    }
    if (eyesClosed && (now - blinkCloseMs) >= BLINK_CLOSE_MS) {
        eyesClosed = false;
        scheduleNextBlink();
    }
    float openFrac = eyesClosed ? 0.0f : 1.0f;

    // ── Micro-motion: whole-eye drift ─────────────────────────────────────────
    if ((int32_t)(now - nextMicroMs) >= 0) {
        nextMicroMs = now + MICRO_MOTION_INTERVAL_MS;
        // ±1 px only — subtle, never distracting
        microDx = (int8_t)((int32_t)(esp_random() % 3) - 1);
        microDy = (int8_t)((int32_t)(esp_random() % 3) - 1);
    }

    // ── Breathing (SITTING) ───────────────────────────────────────────────────
    if (cur == STATE_SITTING && (int32_t)(now - nextBreathMs) >= 0) {
        nextBreathMs  = now + BREATH_INTERVAL_MS;
        breathOffsetY = (breathOffsetY == 0) ? ((esp_random() & 1) ? 1 : -1) : 0;
    }

    // ── Flash (TIMESUP) ───────────────────────────────────────────────────────
    if (cur == STATE_TIMESUP && (now - lastFlashMs) >= TIMESUP_FLASH_MS) {
        lastFlashMs  = now;
        flashVisible = !flashVisible;
    }

    // ── Draw ──────────────────────────────────────────────────────────────────
    switch (cur) {
        case STATE_IDLE:
            drawFaceIdle(openFrac, microDx, microDy);
            break;

        case STATE_SITTING: {
            uint32_t el   = now - sitStartMs;
            uint32_t left = (el < sitDurationMs) ? (sitDurationMs - el + 500) / 1000 : 0;
            drawFaceSitting(openFrac, microDx, microDy, breathOffsetY, left);
            break;
        }

        case STATE_TIMESUP:
            drawFaceTimesUp(flashVisible);
            break;

        case STATE_NOTIFY: {
            float sc = 1.0f;
            if (notifyEntering) {
                uint32_t el = now - notifyEnterStart;
                if (el < NOTIFY_ENTRANCE_MS) {
                    sc = (float)el / NOTIFY_ENTRANCE_MS;
                } else {
                    notifyEntering = false;
                }
            }
            uint32_t t = now - notifyStartMs;   // ms since overlay started

            // Original 4 notification faces (with pop-in scale)
            if      (notifyType == "email")      drawFaceEmail     (sc, microDx, microDy);
            else if (notifyType == "chat")       drawFaceChat      (sc, microDx, microDy);
            else if (notifyType == "alert")      drawFaceAlert     (sc, microDx, microDy);
            // 10 new animation types (time-driven, no pop-in scale needed)
            else if (notifyType == "fly")        drawFaceFly       ();
            else if (notifyType == "cycling")    drawFaceCycling   (t);
            else if (notifyType == "pingpong")   drawFacePingPong  (t);
            else if (notifyType == "sleepy")     drawFaceSleepy    (t);
            else if (notifyType == "suspicious") drawFaceSuspicious(t);
            else if (notifyType == "dizzy")      drawFaceDizzy     (t);
            else if (notifyType == "shy")        drawFaceShy       (t);
            else if (notifyType == "groove")     drawFaceGroove    (t);
            else if (notifyType == "thinking")   drawFaceThinking  (t);
            else if (notifyType == "glitch")     drawFaceGlitch    ();
            else                                 drawFaceGeneric   (sc, microDx, microDy);
            break;
        }
    }

    display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  HTTP handlers
// ─────────────────────────────────────────────────────────────────────────────
static void handleRoot(AsyncWebServerRequest *req) {
    const char* m = "IDLE";
    AppState s = appState;
    if      (s == STATE_SITTING) m = "SITTING";
    else if (s == STATE_TIMESUP) m = "TIMESUP";
    else if (s == STATE_NOTIFY)  m = "NOTIFY";

    String body = "Desk Mochi — Stand-Up Reminder\n"
                  "Current mode: " + String(m) + "\n\n"
                  "Available routes:\n"
                  "  GET  /               — this page\n"
                  "  GET  /status         — JSON status\n"
                  "  POST /pomodoro/start — field: minutes (default 45)\n"
                  "  POST /pomodoro/stop  — reset to IDLE\n"
                  "  POST /notify         — field: type:\n"
                  "    Notifications : email | chat | alert | generic\n"
                  "    Fun animations: fly | cycling | pingpong | sleepy\n"
                  "                    suspicious | dizzy | shy | groove\n"
                  "                    thinking | glitch\n";
    req->send(200, "text/plain", body);
}

static void handleStatus(AsyncWebServerRequest *req) {
    AppState s = appState;
    const char* m = "IDLE";
    if      (s == STATE_SITTING) m = "SITTING";
    else if (s == STATE_TIMESUP) m = "TIMESUP";
    else if (s == STATE_NOTIFY)  m = "NOTIFY";

    uint32_t left = 0;
    AppState check = (s == STATE_NOTIFY) ? underState : s;
    if (check == STATE_SITTING) {
        uint32_t el = millis() - sitStartMs;
        left = (el < sitDurationMs) ? (sitDurationMs - el + 500) / 1000 : 0;
    }
    String nv = (s == STATE_NOTIFY) ? ("\"" + notifyType + "\"") : "null";
    req->send(200, "application/json",
              String("{\"mode\":\"") + m + "\",\"seconds_left\":" + left +
              ",\"notify_type\":" + nv + "}");
}

static void handleStart(AsyncWebServerRequest *req) {
    int minutes = DEFAULT_SIT_MINUTES;
    if (req->hasParam("minutes", true)) {
        int v = req->getParam("minutes", true)->value().toInt();
        if (v > 0) minutes = v;
    }
    sitDurationMs = (uint32_t)minutes * 60UL * 1000UL;
    sitStartMs    = millis();
    nextBreathMs  = millis() + BREATH_INTERVAL_MS;
    breathOffsetY = 0;
    appState      = STATE_SITTING;
    req->send(200, "application/json",
              String("{\"ok\":true,\"minutes\":") + minutes + "}");
}

static void handleStop(AsyncWebServerRequest *req) {
    appState   = STATE_IDLE;
    underState = STATE_IDLE;
    req->send(200, "application/json", "{\"ok\":true,\"mode\":\"IDLE\"}");
}

static void handleNotify(AsyncWebServerRequest *req) {
    String type = "generic";
    if (req->hasParam("type", true)) {
        String v = req->getParam("type", true)->value();
        if (v == "email"   || v == "chat"      || v == "alert"   || v == "generic"  ||
            v == "fly"     || v == "cycling"   || v == "pingpong"|| v == "sleepy"   ||
            v == "suspicious" || v == "dizzy"  || v == "shy"     || v == "groove"   ||
            v == "thinking"   || v == "glitch") {
            type = v;
        }
    }
    notifyType        = type;
    underState        = appState;
    notifyStartMs     = millis();
    notifyEntering    = true;
    notifyEnterStart  = millis();
    nextMicroMs       = millis() + MICRO_MOTION_INTERVAL_MS;
    microDx = microDy = 0;
    appState = STATE_NOTIFY;
    req->send(200, "application/json",
              String("{\"ok\":true,\"type\":\"") + type + "\"}");
}

static void handle404(AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not found");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Desk Mochi booting ===");

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] begin() failed — check wiring/address");
        for (;;) delay(1000);
    }
    display.clearDisplay();
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(24, 20);
    display.print("Desk  Mochi");
    display.setCursor(14, 34);
    display.print("Connecting WiFi...");
    display.display();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] Connecting");
    uint32_t wStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wStart > 20000) {
            Serial.println("\n[WiFi] Timeout — running offline");
            display.clearDisplay();
            display.setCursor(10, 20); display.print("WiFi failed!");
            display.setCursor(10, 34); display.print("Running offline.");
            display.display();
            delay(2000);
            break;
        }
        delay(250);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] " + WiFi.localIP().toString());
        display.clearDisplay();
        display.setCursor(16, 20); display.print("Connected!");
        display.setCursor(10, 34); display.print(WiFi.localIP().toString());
        display.display();
        delay(1500);
    }

    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("[mDNS] " MDNS_HOSTNAME ".local");
    }

    server.on("/",               HTTP_GET,  handleRoot);
    server.on("/status",         HTTP_GET,  handleStatus);
    server.on("/pomodoro/start", HTTP_POST, handleStart);
    server.on("/pomodoro/stop",  HTTP_POST, handleStop);
    server.on("/notify",         HTTP_POST, handleNotify);
    server.onNotFound(handle404);
    server.begin();
    Serial.println("[HTTP] Started on port 80");

    uint32_t now = millis();
    scheduleNextBlink();
    nextMicroMs  = now + MICRO_MOTION_INTERVAL_MS;
    nextBreathMs = now + BREATH_INTERVAL_MS;
    lastFlashMs  = now;
    lastFrameMs  = now;
    Serial.println("[Boot] Done");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main loop — non-blocking
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    uint32_t now = millis();

    // NOTIFY overlay expiry
    if (appState == STATE_NOTIFY && (now - notifyStartMs) >= NOTIFY_DURATION_MS) {
        appState = underState;
    }

    // SITTING → TIME'S UP (primary)
    if (appState == STATE_SITTING && (now - sitStartMs) >= sitDurationMs) {
        appState     = STATE_TIMESUP;
        flashVisible = true;
        lastFlashMs  = now;
    }

    // SITTING → TIME'S UP underneath NOTIFY overlay
    if (appState == STATE_NOTIFY && underState == STATE_SITTING
        && (now - sitStartMs) >= sitDurationMs) {
        underState = STATE_TIMESUP;
    }

    // Render at ~30 fps
    if ((now - lastFrameMs) >= FRAME_INTERVAL_MS) {
        lastFrameMs = now;
        renderFrame();
    }
}
