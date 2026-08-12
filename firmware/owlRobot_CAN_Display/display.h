#pragma once
#include <Arduino.h>
#include <IPAddress.h>

class DashboardDisplay {
public:
  // ---- Status/Enums ----
  enum SatStatus { Fix, Float, Invalid };

  // ---- Lebenszyklus ----
  void begin(int backlightPin = -1, bool activeHigh = true);
  void loop();
  void setAutoRefreshInterval(uint32_t ms);
  void forceRefresh();

  // ---- Utils ----
  static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);

  // ---- Setter (linke Spalte) ----
  void setSat(SatStatus status, uint8_t used, uint8_t total);
  void setSat(const String& status, uint8_t used, uint8_t total);
  void setSatUnavailable();

  void setRTK(float seconds, uint8_t decimals);
  void setRTKstr(const String& s);
  void setRTKUnavailable();

  void setVolt(float volts, uint8_t decimals);
  void setVoltStr(const String& s);
  void setVoltUnavailable();

  void setAmp(float amps, uint8_t decimals);
  void setAmpStr(const String& s);
  void setAmpUnavailable();
  void setUltrasonicAlert(uint8_t sensorId, bool active, uint16_t distanceMm);

  // ---- Setter (rechte Spalte) ----
  void setWifiSignal(int16_t dbm);
  void setIP(const ::IPAddress& ip);
  void setIP(const String& ipString);

  void setMap(uint16_t count, uint8_t percent);
  void setMapUnavailable();
  void setState(const String& s);

  // ---- Message/Banner ----
  void showBanner();
  void showMessage(const String& msg, uint16_t color = 0x0000);
  void setMessageScrollSpeed(uint8_t pixelsPerStep, uint16_t intervalMs = 16);

  // ---- Touch/Overlay ----
  bool onTouch(int16_t x, int16_t y);  // true = Event verarbeitet

  // (Kompatibilität – zeigen jetzt das allgemeine Overlay an)
  void toggleSatDetails();
  void showSatDetails(bool show);

private:
  // ---- interner Aufbau/Redraw ----
  void drawStaticBackground();
  void redrawMessageArea();
  void redrawAll(bool force);

  // ---- Overlay-Framework ----
  void drawSatDetailsOverlay(); // Kompatibilitäts-Shim
  enum class OverlayKind { None, Sat, RTK, Wifi, IP, Volt, Amp, Map, State };
  void showOverlay(OverlayKind kind, bool show);
  void drawOverlay(OverlayKind kind);
  void drawOverlayBox(const String& title,
                      const String& bigLine,
                      const String& subLine,
                      uint16_t bigColor = 0x0000 /*TFT_BLACK*/);

  // ---- Konfiguration ----
  int   blPin_        = -1;
  bool  blActiveHigh_ = true;

  // ---- Auto-Refresh ----
  uint32_t     autoRefreshMs_ = 30000;
  unsigned long tAuto_        = 0;

  // ---- Overlay-Status ----
  OverlayKind overlay_ = OverlayKind::None;

  // ---- Daten (linke Spalte) ----
  String  satStatusLabel_;
  uint8_t satUsed_  = 0;
  uint8_t satTotal_ = 0;
  String  rtk_;
  float   voltVal_ = NAN;  // für Farblogik
  String  volt_;
  String  amp_;

  // ---- Daten (rechte Spalte) ----
  bool    wifiValid_   = false;
  int16_t wifiDbm_     = -127;
  uint8_t wifiPercent_ = 0;
  String  ipStr_;

  uint16_t mapCount_   = 0;
  uint8_t  mapPercent_ = 0;

  String  state_;
  bool    satValid_   = false;
  bool    rtkValid_   = false;
  bool    mapValid_   = false;
  bool    voltValid_  = false;
  bool    ampValid_   = false;
  bool    ultrasonicLeftActive_  = false;
  bool    ultrasonicRightActive_ = false;
  uint16_t ultrasonicLeftDistance_  = 0;
  uint16_t ultrasonicRightDistance_ = 0;
  unsigned long ultrasonicLeftUpdatedMs_  = 0;
  unsigned long ultrasonicRightUpdatedMs_ = 0;
  bool    ultrasonicMessageActive_ = false;
  String  ultrasonicMessage_;
  uint16_t ultrasonicMessageColor_ = 0x0000;
  unsigned long ultrasonicLastUpdateMs_ = 0;
  unsigned long ultrasonicInactiveSinceMs_ = 0;
  unsigned long ultrasonicDebugLastPrintMs_ = 0;
  bool ultrasonicTimeoutLogged_ = false;
  String  generalMessage_;
  uint16_t generalMessageColor_ = 0;
  unsigned long generalMessageShownMs_ = 0;
  bool    generalMessageValid_ = false;
};
