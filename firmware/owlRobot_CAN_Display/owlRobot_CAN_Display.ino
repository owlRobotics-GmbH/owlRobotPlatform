/*
owlRobot_CAN_Display
- CPU library: Raspberry Pi Pico/RP2040 Earle Philhower 3.3.2
- (do not install: PNGdec von Larry Bank 1.0.1)
- (do not install: TFT_eSPI von Boder  2.5.43)
- (do not install: XPT2046_Touchscreen 1.4.0)
*/

#include <Arduino.h>
#include <SPI.h>
#include "src/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
#include "display.h"
#include "canMsgHandler.h"
#include "owlcan.h"
#include "display_management.h"

// ---------- PINBELEGUNG (deine Vorgabe) ----------
#define BACKLIGHT_PIN   2     // aktiv HIGH
#define TOUCH_CS        10    // XPT2046 CS
#define TOUCH_IRQ       11    // XPT2046 IRQ (LOW bei Touch)

// Touch: Rotation = 3 (bei dir korrekt)
#define TS_ROTATION     3
// Kalibrierung (roh) für Rotation=3 -> ggf. feintunen
#define TS_MINX   200
#define TS_MAXX  3900
#define TS_MINY   200
#define TS_MAXY  3900

DashboardDisplay Display;
XPT2046_Touchscreen Touch(TOUCH_CS, TOUCH_IRQ);
CanMsgHandler canHandler(Display);

static uint8_t touchEventCounter = 0;
static bool touchWasDown = false;
static uint32_t lastTouchEventMs = 0;

static void sendTouchEvent(int16_t x, int16_t y) {
  canNodeType_t node{};
  node.sourceAndDest.sourceNodeID = OWL_DISPLAY_NODE_ID;
  node.sourceAndDest.destNodeID = 63; // broadcast

  can_frame_t frame{};
  frame.can_id = OWL_DISPLAY_MSG_ID;
  frame.can_dlc = 8;
  frame.data[0] = node.byteVal[0];
  frame.data[1] = node.byteVal[1];
  frame.data[2] = can_cmd_info;
  frame.data[3] = owldisplay::can_val_touch_event;
  frame.data[4] = 1; // tap/down event
  frame.data[5] = static_cast<uint8_t>(constrain(x / 2, 0, 159));
  frame.data[6] = static_cast<uint8_t>(constrain(y, 0, 239));
  frame.data[7] = ++touchEventCounter;

  can.write(frame);
  can.processTxFifo();
}

// ===== Touch lesen & an Display übergeben =====
static inline void handleTouch() {
  bool touched = Touch.touched() || (digitalRead(TOUCH_IRQ) == LOW);
  if (!touched) {
    touchWasDown = false;
    return;
  }

  TS_Point p = Touch.getPoint();   // bereits rotiert durch setRotation()

  int16_t sx = map(p.x, TS_MINX, TS_MAXX, 0, 320);
  int16_t sy = map(p.y, TS_MINY, TS_MAXY, 0, 240);
  if (sx < 0) sx = 0; if (sx > 319) sx = 319;
  if (sy < 0) sy = 0; if (sy > 239) sy = 239;

  const uint32_t now = millis();
  const bool newTap = !touchWasDown && (now - lastTouchEventMs >= 250);
  touchWasDown = true;
  if (!newTap) return;

  lastTouchEventMs = now;
  sendTouchEvent(sx, sy);

  if (Display.onTouch(sx, sy)) {
    delay(120); // kleines Debounce
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("START");

  // Display + Backlight
  Display.begin(BACKLIGHT_PIN, true);
  Display.setAutoRefreshInterval(30000);
  Display.setMessageScrollSpeed(3, 16);

  // Touch
  pinMode(TOUCH_IRQ, INPUT_PULLUP);
  Touch.begin();                // shared SPI0
  Touch.setRotation(TS_ROTATION);

  // Startwerte: klare Placeholder bis echte CAN-Daten eintreffen
  Display.setSatUnavailable();
  Display.setRTKUnavailable();
  Display.setWifiSignal(-127);
  Display.setIP(String("No IP"));
  Display.setVoltUnavailable();
  Display.setAmpUnavailable();
  Display.setMapUnavailable();
  Display.setState("No Op");
  Display.showMessage("Bereit.");

  if (!canHandler.begin()) {
    Serial.println("CAN initialisation failed");
  } else {
    Serial.println("CAN ready");
    Serial.print("Listening on CAN ID ");
    Serial.println(OWL_DISPLAY_MSG_ID);
    Serial.print("Display node ID ");
    Serial.println(OWL_DISPLAY_NODE_ID);
    canHandler.enableDebug(false);
  }
  displayManagement.begin();
  Serial.println("READY");
}

void loop() {
  Display.loop();      // Laufschrift/Auto-Refresh
  handleTouch();       // Overlays öffnen/schließen
  canHandler.process();
}
