#include <Arduino.h>
#include <IPAddress.h>
#include "display.h"

#include "src/TFT_eSPI/TFT_eSPI.h"
#include "src/PNGdec/src/PNGdec.h"
#include "bitmaps.h"          // erwartet: background[], owlBanner[] (PROGMEM)

namespace Layout {
  constexpr int boxW=90, boxH=40;
  constexpr int L=60, R=175;
  constexpr int Y0=1, Y1=45, Y2=89, Y3=133;

  constexpr int MsgY=177, MsgH=63, ScreenW=320, ScreenH=240;
}

namespace Theme {
  constexpr float kVmin = 20.0f;
  constexpr float kVmax = 29.2f;

  constexpr uint8_t FixR=0,   FixG=110, FixB=0;
  constexpr uint8_t FloR=200, FloG=120, FloB=0;
  constexpr uint8_t InvR=180, InvG=0,   InvB=0;
}

namespace {
  constexpr unsigned long kUltrasonicHoldMs = 3000;
  constexpr unsigned long kGeneralMessageHoldMs = 45000;
  constexpr bool          kDebugUltrasonic   = true;
}

// ---------- Low-level TFT/PNG ----------
static TFT_eSPI tft;
static PNG png;
static int16_t pngX = 0, pngY = 0;

static void pngDraw(PNGDRAW *pDraw) {
  static uint16_t lineBuf[320];
  png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);
  tft.pushImage(pngX, pngY + pDraw->y, pDraw->iWidth, 1, lineBuf);
}
static bool drawPNGFromProgmem(const uint8_t* data, size_t len, int16_t x, int16_t y) {
  pngX = x; pngY = y;
  if (png.openFLASH((uint8_t*)data, len, pngDraw) == PNG_SUCCESS) {
    tft.startWrite(); png.decode(NULL, 0); tft.endWrite(); png.close();
    return true;
  }
  return false;
}

// ---------- Helper ----------
namespace gfx {
  static String fmtThousands(uint32_t v){
    String s = String(v), out; int n = s.length();
    for (int i=0;i<n;i++){ if (i && ((n-i)%3==0)) out += '.'; out += s[i]; }
    return out;
  }
  static uint16_t colorForSat(const String& s){
    if (s.equalsIgnoreCase("fix"))   return DashboardDisplay::rgb565(Theme::FixR, Theme::FixG, Theme::FixB);
    if (s.equalsIgnoreCase("float")) return DashboardDisplay::rgb565(Theme::FloR, Theme::FloG, Theme::FloB);
    return DashboardDisplay::rgb565(Theme::InvR, Theme::InvG, Theme::InvB);
  }
  static uint16_t colorForVolt(float v){
    if (isnan(v)) return 0x0000; // schwarz
    float t = (v - Theme::kVmin) / (Theme::kVmax - Theme::kVmin);
    if (t < 0) t = 0; if (t > 1) t = 1;
    uint8_t r = (uint8_t)(200.0f * (1.0f - t));
    uint8_t g = (uint8_t)(110.0f * t);
    return DashboardDisplay::rgb565(r, g, 0);
  }
  static uint16_t colorForSignal(uint8_t percent){
    if (percent >= 67) return DashboardDisplay::rgb565(0, 160, 0);
    if (percent >= 34) return DashboardDisplay::rgb565(200, 160, 0);
    return DashboardDisplay::rgb565(200, 0, 0);
  }
}

// ---------- UI-Bausteine ----------
struct Field {
  int16_t x,y,w,h; uint8_t size; uint16_t fg,bg;
  Field():x(0),y(0),w(0),h(0),size(1),fg(TFT_BLACK),bg(TFT_WHITE) {}
  Field(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint8_t _s=2,uint16_t _fg=TFT_BLACK,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),size(_s),fg(_fg),bg(_bg) {}
  void clear() const { tft.fillRect(x,y,w,h,bg); }
  void print(const String&s,int16_t dx=4,int16_t dy=-1) const {
    int16_t useDy=(dy<0)?(int16_t)((h-8)/2):dy;
    clear();
    tft.setTextSize(size);
    tft.setTextColor(fg,bg);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(s, x+dx, y+useDy);
  }
};
struct CachedField : Field {
  mutable String last; using Field::Field;
  void show(const String&s,int16_t dx=4,int16_t dy=-1,bool force=false) const {
    if(!force && s==last) return; Field::print(s,dx,dy); last=s;
  }
  void invalidate() const { last = "\x01"; }
};
struct CenteredField : Field {
  mutable String last; using Field::Field;
  void show(const String& s, bool force=false) const {
    if(!force && s==last) return;
    tft.fillRect(x,y,w,h,bg);
    tft.setTextFont(1);
    tft.setTextSize(size);
    tft.setTextColor(fg,bg);
    tft.setTextDatum(MC_DATUM);

    int lineCount = 1;
    for (uint16_t i = 0; i < s.length(); ++i) {
      if (s[i] == '\n') ++lineCount;
    }

    const int lineHeight = 8 * size;
    const int lineSpacing = 4;
    const int totalHeight = lineCount * lineHeight + (lineCount - 1) * lineSpacing;
    const int16_t centerX = x + w/2;
    int16_t currentY = y + (h - totalHeight) / 2 + lineHeight / 2;

    int start = 0;
    for (int line = 0; line < lineCount; ++line) {
      int end = s.indexOf('\n', start);
      if (end < 0) end = s.length();
      String segment = s.substring(start, end);
      tft.drawString(segment, centerX, currentY);
      currentY += lineHeight + lineSpacing;
      start = end + 1;
    }

    last = s;
  }
  void show(const String& s, int16_t /*dx*/, int16_t /*dy*/, bool force=false) const { show(s, force); }
  void invalidate() const { last = "\x01"; }
};
struct TwoLineBox {
  int16_t x,y,w,h; uint8_t size; uint16_t fg,bg; String last1,last2;
  TwoLineBox():x(0),y(0),w(0),h(0),size(1),fg(TFT_BLACK),bg(TFT_WHITE) {}
  TwoLineBox(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint8_t _s=1,uint16_t _fg=TFT_BLACK,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),size(_s),fg(_fg),bg(_bg) {}
  void show(const String&l1,const String&l2,bool force=false){
    if(!force && l1==last1 && l2==last2) return;
    tft.fillRect(x,y,w,h,bg);
    tft.setTextSize(size); tft.setTextColor(fg,bg); tft.setTextDatum(TL_DATUM);
    const int16_t dy=4;
    tft.drawString(l1, x+4, y+dy);
    tft.drawString(l2, x+4, y+dy+16);
    last1=l1; last2=l2;
  }
  void invalidate(){ last1="\x01"; last2="\x02"; }
};
struct SatBox {
  int16_t x,y,w,h; uint16_t fg,bg; uint8_t bigSize, smallSize; int16_t statusOffsetX;
  String lastStatus; uint8_t lastUsed=255, lastTotal=255;
  SatBox(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint8_t _big=2,uint8_t _small=2,
         uint16_t _fg=TFT_BLACK,uint16_t _bg=TFT_WHITE,int16_t _offsetX=-14)
  :x(_x),y(_y),w(_w),h(_h),fg(_fg),bg(_bg),bigSize(_big),smallSize(_small),statusOffsetX(_offsetX) {}
  void show(const String& status, uint8_t used, uint8_t total, bool force=false, uint16_t statusColor=0){
    if(!force && status==lastStatus && used==lastUsed && total==lastTotal) return;
    tft.fillRect(x,y,w,h,bg);

    uint8_t textSize = bigSize;
    tft.setTextSize(textSize); tft.setTextDatum(MC_DATUM);
    tft.setTextColor(statusColor ? statusColor : fg, bg);
    bool placeholder = status.equalsIgnoreCase("No Sat") || status.equalsIgnoreCase("No GNSS");
    int16_t cx = x + w/2 + (placeholder ? 0 : statusOffsetX);
    tft.drawString(status, cx, y + h/2);

    tft.setTextSize(smallSize); tft.setTextColor(fg, bg);
    tft.setTextDatum(TR_DATUM);
    if (used || total) {
      tft.drawString(String(used),  x + w - 3, y + 3);
      tft.setTextDatum(BR_DATUM);
      tft.drawString(String(total), x + w - 3, y + h - 3);
    }

    lastStatus=status; lastUsed=used; lastTotal=total;
  }
  void invalidate(){ lastStatus="\x01"; lastUsed=254; lastTotal=254; }
};

// —— MessageArea mit Laufschrift via Sprite —— //
struct MessageArea {
  int16_t x,y,w,h; uint16_t bg;
  String current;
  uint16_t msgColor = 0x0000;
  bool marquee = false;
  int  scrollX = 0;
  int  textWpx = 0;
  uint8_t size = 2;
  unsigned long lastTick = 0;

  uint8_t  stepPx  = 3;
  uint16_t stepMs  = 16;

  TFT_eSprite spr = TFT_eSprite(&tft);
  bool sprReady = false;

  MessageArea():x(0),y(0),w(0),h(0),bg(TFT_WHITE) {}
  MessageArea(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),bg(_bg) {}

  void clear() const { tft.fillRect(x,y,w,h,bg); }
  void destroySprite(){ if (sprReady) { spr.deleteSprite(); sprReady = false; } }

  void buildSpriteForText(){
    destroySprite();
    tft.setTextFont(1); tft.setTextSize(size);
    textWpx = tft.textWidth(current.c_str());
    int sprW = textWpx + 16; if (sprW < 16) sprW = 16; if (sprW > 1200) sprW = 1200;
    int sprH = h;
    if (spr.createSprite(sprW, sprH) == nullptr) { sprReady = false; return; }
    sprReady = true;

    spr.fillSprite(bg);
    spr.setTextFont(1);
    spr.setTextSize(size);
    spr.setTextColor(msgColor, bg);
    spr.setTextDatum(TL_DATUM);

    int16_t baseY = (h - 8*size)/2;
    spr.drawString(current, 0, baseY);

    marquee = (textWpx > (w - 8));
    scrollX = marquee ? (w - 1) : (w - textWpx) / 2;
    lastTick = millis();
  }

  void set(const String& msg, uint16_t color, bool force=false){
    if (!force && msg == current && color == msgColor) return;
    msgColor = color; current = msg;

    if (current.length() == 0){ destroySprite(); renderBanner(); return; }
    buildSpriteForText(); redraw(true);
  }

  void renderBanner(){
    current = "";
    msgColor = 0x0000;
    marquee = false; destroySprite(); clear();
    drawPNGFromProgmem(owlBanner, sizeof(owlBanner), x, y);
  }

  void renderTextBox(const String& text, uint16_t color){
    marquee = false;
    destroySprite();
    clear();

    tft.setTextFont(1);
    uint8_t drawSize = size;
    tft.setTextSize(drawSize);
    while (drawSize > 1 && tft.textWidth(text.c_str()) > (w - 8)) {
      drawSize--;
      tft.setTextSize(drawSize);
    }
    tft.setTextColor(color, bg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, x + w/2, y + h/2);
  }

  void update(unsigned long now){
    if (!marquee || current.length()==0 || !sprReady) return;
    if (now - lastTick < stepMs) return;
    lastTick = now;

    scrollX -= stepPx;
    if (scrollX + textWpx <= 0) scrollX = w;
    redraw(false);
  }

  void redraw(bool /*force*/){
    if (current.length()==0){ renderBanner(); return; }
    if (sprReady) {
      if (scrollX >= w) scrollX = w - 1;
      tft.setViewport(x, y, w, h);
      tft.fillRect(x, y, w, h, bg);
      spr.pushSprite(x + scrollX, y);
      tft.resetViewport();
      return;
    }
    tft.fillRect(x,y,w,h,bg);
    tft.setTextColor(msgColor, bg);
    tft.setTextFont(1);
    tft.setTextSize(size);
    if (!marquee){
      tft.setTextDatum(MC_DATUM);
      tft.drawString(current, x + w/2, y + h/2);
    } else {
      tft.setTextDatum(TL_DATUM);
      int16_t baseY = y + (h - 8*size)/2;
      tft.drawString(current, x + scrollX, baseY);
    }
  }
};

struct IpBox {
  int16_t x,y,w,h; uint16_t fg,bg; String last;
  IpBox(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint16_t _fg=TFT_BLACK,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),fg(_fg),bg(_bg) {}
  void show(const String& ip, bool force=false){
    if(!force && ip==last) return;
    tft.fillRect(x,y,w,h,bg);
    tft.setTextWrap(false);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(fg,bg);

    uint8_t size = 2;
    tft.setTextFont(1);
    tft.setTextSize(size);
    int tw = tft.textWidth(ip.c_str());
    if (tw > (w - 6)) { size = 1; tft.setTextSize(size); }

    tft.drawString(ip, x + w/2, y + h/2);
    last = ip;
  }
  void invalidate(){ last = "\x01"; }
};

struct MapBox {
  int16_t x,y,w,h; uint16_t fg,bg;
  uint16_t lastCount = 0xFFFF;
  uint8_t  lastPc    = 0xFF;
  bool     lastValid = true;

  MapBox(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint16_t _fg=TFT_BLACK,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),fg(_fg),bg(_bg) {}

  void show(uint16_t count, uint8_t percent, bool valid, bool force=false){
    if(!force && count==lastCount && percent==lastPc && valid == lastValid) return;

    tft.fillRect(x,y,w,h,bg);

    const uint16_t labelGrey = DashboardDisplay::rgb565(120,120,120);
    tft.setTextFont(1);

    if (!valid) {
      tft.setTextColor(labelGrey, bg);
      tft.setTextSize(1);
      tft.setTextDatum(TL_DATUM);
      tft.drawString("Map", x+3, y+2);

      tft.setTextFont(1);
      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(2);
      tft.setTextColor(labelGrey, bg);
      tft.drawString("No Map", x + w/2, y + h/2);
    } else {
      tft.setTextColor(labelGrey, bg);
      tft.setTextSize(1);
      tft.setTextDatum(TL_DATUM);
      tft.drawString("Map", x+3, y+2);

      String cnt = gfx::fmtThousands(count);
      uint8_t cntSize = 2;
      tft.setTextSize(cntSize);
      tft.setTextDatum(TR_DATUM);
      if (tft.textWidth(cnt.c_str()) > (w - 10)) { cntSize = 2; tft.setTextSize(cntSize); }
      tft.drawString(cnt, x + w - 3, y + 6);

      String pct = String(percent) + "%";
      uint16_t pcColor = (percent >= 100) ? DashboardDisplay::rgb565(0,110,0) : TFT_BLACK;

      tft.setTextSize(1); int doneW = tft.textWidth("done");
      uint8_t pctSize = 2;
      tft.setTextSize(pctSize);
      int avail = w - 6 - doneW - 2;
      if (tft.textWidth(pct.c_str()) > avail) { pctSize = 2; tft.setTextSize(pctSize); }

      tft.setTextDatum(BL_DATUM);
      tft.setTextColor(pcColor, bg);
      tft.drawString(pct, x + 3, y + h - 3);

      tft.setTextSize(1);
      tft.setTextDatum(BR_DATUM);
      tft.setTextColor(labelGrey, bg);
      tft.drawString("done", x + w - 3, y + h - 3);
    }

    lastCount = count; lastPc = percent;
    lastValid = valid;
  }
  void invalidate(){ lastCount = 0xFFFE; lastPc = 0xFE; lastValid = !lastValid; }
};

struct VoltBox {
  int16_t x,y,w,h; uint16_t bg;
  String lastText; uint16_t lastColor = 0xFFFF;
  VoltBox(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),bg(_bg) {}
  void show(const String& text, uint16_t color, bool force=false){
    if(!force && text == lastText && color == lastColor) return;
    tft.fillRect(x,y,w,h,bg);
    tft.setTextFont(1);
    uint8_t size = 2;
    tft.setTextSize(size);
    while (size > 1 && tft.textWidth(text.c_str()) > (w - 8)) {
      size--;
      tft.setTextSize(size);
    }
    tft.setTextColor(color, bg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, x + w/2, y + h/2);
    lastText = text; lastColor = color;
  }
  void invalidate(){ lastText = "\x01"; lastColor = 0xFFFE; }
};

struct WifiBox {
  int16_t x,y,w,h; uint16_t bg;
  bool lastValid = false;
  int16_t lastDbm = 32767;
  uint8_t lastPercent = 255;
  WifiBox(int16_t _x,int16_t _y,int16_t _w,int16_t _h,uint16_t _bg=TFT_WHITE)
  :x(_x),y(_y),w(_w),h(_h),bg(_bg) {}
  void show(bool valid, int16_t dbm, uint8_t percent, bool force=false){
    if(!force && valid == lastValid && dbm == lastDbm && percent == lastPercent) return;
    tft.fillRect(x,y,w,h,bg);

    const int16_t margin = 4;
    const uint16_t frameColor = DashboardDisplay::rgb565(120,120,120);
    const uint16_t bgFill = DashboardDisplay::rgb565(230,230,230);
    const uint16_t warnColor = DashboardDisplay::rgb565(200, 0, 0);
    const uint16_t warnFill  = DashboardDisplay::rgb565(255, 225, 225);

    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(valid ? TFT_BLACK : warnColor, bg);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(valid ? "WiFi" : "WiFi OFF", x + margin, y + 2);

    String pctStr = valid ? (String(percent) + "%") : String("OFF");
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(valid ? TFT_BLACK : warnColor, bg);
    tft.drawString(pctStr, x + w - margin, y + 2);

    int16_t barX = x + margin;
    int16_t barY = y + 18;
    int16_t barW = w - 2 * margin;
    if (barW < 6) barW = 6;
    int16_t barH = h - (barY - y) - margin;
    if (barH < 6) barH = 6;

    const uint16_t boxFrame = valid ? frameColor : warnColor;
    const uint16_t boxFill  = valid ? bgFill : warnFill;
    tft.drawRoundRect(barX, barY, barW, barH, 3, boxFrame);
    tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, boxFill);

    if (valid && percent > 0) {
      uint16_t fillColor = gfx::colorForSignal(percent);
      int16_t fillW = (barW - 2) * percent / 100;
      if (fillW > barW - 2) fillW = barW - 2;
      if (fillW > 0) {
        tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, fillColor);
      }
    } else if (!valid) {
      tft.drawLine(barX + 2, barY + 2, barX + barW - 3, barY + barH - 3, warnColor);
      tft.drawLine(barX + 2, barY + barH - 3, barX + barW - 3, barY + 2, warnColor);
    }

    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(valid ? TFT_BLACK : warnColor, bg);
    String detail = valid ? (String(dbm) + " dBm") : String("No WiFi link");
    tft.drawString(detail, barX, y + h - 2);

    lastValid = valid;
    lastDbm = dbm;
    lastPercent = percent;
  }
  void invalidate(){
    lastDbm = 32767;
    lastPercent = 255;
    lastValid = !lastValid;
  }
};

// ---------- Layout / Instanzen ----------
static SatBox        satBox(Layout::L, Layout::Y0, Layout::boxW, Layout::boxH, 2, 2, TFT_BLACK, TFT_WHITE, -14);
static CenteredField fRTK (Layout::L, Layout::Y1, Layout::boxW, Layout::boxH, 2);
static WifiBox       wifiBox(Layout::R, Layout::Y0, Layout::boxW, Layout::boxH,   TFT_WHITE);
static IpBox         ipBox (Layout::R, Layout::Y1, Layout::boxW, Layout::boxH,     TFT_BLACK, TFT_WHITE);

static VoltBox       voltBox(Layout::L, Layout::Y2, Layout::boxW, Layout::boxH,     TFT_WHITE);
static CenteredField fAmp  (Layout::L, Layout::Y3, Layout::boxW, Layout::boxH, 2);
static MapBox        mapBox(Layout::R, Layout::Y2, Layout::boxW, Layout::boxH,     TFT_BLACK, TFT_WHITE);
static CenteredField fStat (Layout::R, Layout::Y3, Layout::boxW, Layout::boxH, 2);

static MessageArea   msgArea(0, Layout::MsgY, Layout::ScreenW, Layout::MsgH, TFT_WHITE);

// ---------- DashboardDisplay ----------
uint16_t DashboardDisplay::rgb565(uint8_t r, uint8_t g, uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void DashboardDisplay::begin(int backlightPin, bool activeHigh){
  blPin_ = backlightPin; blActiveHigh_ = activeHigh;
  if (blPin_ >= 0){ pinMode(blPin_, OUTPUT); digitalWrite(blPin_, blActiveHigh_ ? HIGH : LOW); }

  tft.init();
  tft.setRotation(1);
  tft.setTextFont(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  drawStaticBackground();

  satValid_ = false;
  satStatusLabel_ = "No Sat";
  satUsed_ = 0;
  satTotal_ = 0;
  rtkValid_ = false;
  rtk_ = "No RTK";
  voltValid_ = false;
  voltVal_ = NAN;
  volt_ = "--.- V";
  ampValid_ = false;
  amp_ = "--.- A";
  mapValid_ = false;
  mapCount_ = 0;
  mapPercent_ = 0;
  wifiValid_ = false;
  wifiDbm_ = -127;
  wifiPercent_ = 0;
  ipStr_ = "";

  // Erstanzeige forcieren
  satBox.invalidate(); fRTK.invalidate(); voltBox.invalidate(); fAmp.invalidate();
  mapBox.invalidate(); fStat.invalidate(); wifiBox.invalidate(); ipBox.invalidate();
  msgArea.set("", 0x0000);
  redrawMessageArea();

  redrawAll(true);
  tAuto_ = millis();
}

void DashboardDisplay::setAutoRefreshInterval(uint32_t ms){ autoRefreshMs_ = ms; }

void DashboardDisplay::loop(){
  unsigned long now = millis();

  // Solange ein Overlay offen ist: Nichts am Basisdisplay neu zeichnen
  // und Laufschrift pausieren. Timer frisch halten, damit danach kein Sprung kommt.
  if (overlay_ != OverlayKind::None) {
    tAuto_ = now;
    return;
  }

  if (ultrasonicMessageActive_) {
    bool leftActive = ultrasonicLeftActive_ &&
                      (now - ultrasonicLeftUpdatedMs_ <= kUltrasonicHoldMs);
    bool rightActive = ultrasonicRightActive_ &&
                       (now - ultrasonicRightUpdatedMs_ <= kUltrasonicHoldMs);
    if (!leftActive && ultrasonicLeftActive_) {
      if (kDebugUltrasonic) {
        Serial.print("[ULTRA] L timeout (last update ");
        Serial.print(now - ultrasonicLeftUpdatedMs_);
        Serial.println(" ms ago)");
      }
      ultrasonicLeftActive_ = false;
    }
    if (!rightActive && ultrasonicRightActive_) {
      if (kDebugUltrasonic) {
        Serial.print("[ULTRA] R timeout (last update ");
        Serial.print(now - ultrasonicRightUpdatedMs_);
        Serial.println(" ms ago)");
      }
      ultrasonicRightActive_ = false;
    }

    bool ultrasonicCurrentlyActive = leftActive || rightActive;
    if (!ultrasonicCurrentlyActive && ultrasonicInactiveSinceMs_ == 0) {
      ultrasonicInactiveSinceMs_ = now;
      ultrasonicDebugLastPrintMs_ = 0;
    }
    if (ultrasonicCurrentlyActive) {
      ultrasonicTimeoutLogged_ = false;
    }
    if (!ultrasonicCurrentlyActive && kDebugUltrasonic && ultrasonicInactiveSinceMs_ != 0) {
      if (ultrasonicDebugLastPrintMs_ == 0 || now - ultrasonicDebugLastPrintMs_ >= 1000) {
        unsigned long elapsed = now - ultrasonicInactiveSinceMs_;
        Serial.print("[ULTRA] inactive for ");
        Serial.print(elapsed / 1000);
        Serial.print('.');
        Serial.print((elapsed % 1000) / 100);
        Serial.println(" s");
        ultrasonicDebugLastPrintMs_ = now;
      }
    }
    if (!ultrasonicCurrentlyActive &&
        ultrasonicMessageActive_ &&
        ultrasonicLastUpdateMs_ != 0 &&
        (now - ultrasonicLastUpdateMs_ >= kUltrasonicHoldMs)) {
      if (!ultrasonicTimeoutLogged_) {
        if (kDebugUltrasonic) {
          Serial.println("[ULTRA] timeout reached, waiting to clear window");
        }
        ultrasonicTimeoutLogged_ = true;
      }
      if (kDebugUltrasonic) {
        Serial.println("[ULTRA] CLEAR message (timeout reached)");
      }
      ultrasonicMessageActive_ = false;
      ultrasonicMessage_ = "";
      ultrasonicLastUpdateMs_ = 0;
      msgArea.renderBanner();
    }
  } else {
    ultrasonicInactiveSinceMs_ = 0;
    ultrasonicDebugLastPrintMs_ = 0;
    ultrasonicTimeoutLogged_ = false;
  }

  if (!ultrasonicMessageActive_ &&
      generalMessageValid_ &&
      (now - generalMessageShownMs_ >= kGeneralMessageHoldMs)) {
    generalMessage_ = "";
    generalMessageValid_ = false;
    generalMessageShownMs_ = 0;
    msgArea.renderBanner();
  }

  // Nur wenn KEIN Overlay offen ist, darf die Laufschrift animieren
  msgArea.update(now);

  if (autoRefreshMs_ == 0) return;
  if (now - tAuto_ >= autoRefreshMs_) {
    tAuto_ = now;
    redrawAll(true);
  }
}

void DashboardDisplay::forceRefresh(){
  drawStaticBackground();
  satBox.invalidate(); fRTK.invalidate(); voltBox.invalidate(); fAmp.invalidate();
  mapBox.invalidate(); fStat.invalidate(); wifiBox.invalidate(); ipBox.invalidate();
  redrawAll(true);
  redrawMessageArea();
}

void DashboardDisplay::drawStaticBackground(){
  tft.fillScreen(TFT_WHITE);
  drawPNGFromProgmem(background, sizeof(background), 0, 0);

  tft.fillRect(Layout::L, Layout::Y0, Layout::boxW, Layout::boxH, TFT_WHITE);
  tft.fillRect(Layout::L, Layout::Y1, Layout::boxW, Layout::boxH, TFT_WHITE);
  tft.fillRect(Layout::L, Layout::Y2, Layout::boxW, Layout::boxH, TFT_WHITE);
  tft.fillRect(Layout::L, Layout::Y3, Layout::boxW, Layout::boxH, TFT_WHITE);

  tft.fillRect(Layout::R, Layout::Y0, Layout::boxW, Layout::boxH, TFT_WHITE);
  tft.fillRect(Layout::R, Layout::Y1, Layout::boxW, Layout::boxH, TFT_WHITE);
  tft.fillRect(Layout::R, Layout::Y2, Layout::boxW, Layout::boxH, TFT_WHITE);
  tft.fillRect(Layout::R, Layout::Y3, Layout::boxW, Layout::boxH, TFT_WHITE);

  redrawMessageArea();
}

void DashboardDisplay::redrawMessageArea(){
  if (ultrasonicMessageActive_) {
    msgArea.renderTextBox(ultrasonicMessage_, ultrasonicMessageColor_);
  } else if (generalMessageValid_) {
    const unsigned long now = millis();
    if (now - generalMessageShownMs_ < kGeneralMessageHoldMs) {
      msgArea.set(generalMessage_, generalMessageColor_, true);
    } else {
      generalMessage_ = "";
      generalMessageValid_ = false;
      generalMessageShownMs_ = 0;
      msgArea.renderBanner();
    }
  } else {
    msgArea.renderBanner();
  }
}

void DashboardDisplay::redrawAll(bool force){
  tft.startWrite();

  String satLabel;
  if (satValid_) {
    satLabel = satStatusLabel_.length()? satStatusLabel_ : String("Fix");
  } else {
    satLabel = "No Sat";
  }
  const uint8_t satUsed = satValid_ ? satUsed_ : 0;
  const uint8_t satTotal = satValid_ ? satTotal_ : 0;
  const uint16_t satColor = satValid_ ? gfx::colorForSat(satLabel)
                                      : rgb565(140,140,140);
  satBox.show(satLabel, satUsed, satTotal, force, satColor);

  const String rtkLabel = rtkValid_ ? (rtk_.length()? rtk_ : String("0.0 s"))
                                    : String("No RTK");
  if (overlay_ == OverlayKind::None) {
    fRTK.show(rtkLabel, force);
  }

  wifiBox.show(wifiValid_, wifiDbm_, wifiPercent_, force);

  ipBox.show(ipStr_.length()? ipStr_ : String("No IP"), force);

  float v = isnan(voltVal_) ? Theme::kVmax : voltVal_;
  uint16_t vcol = voltValid_ ? gfx::colorForVolt(v) : rgb565(140,140,140);
  const String voltLabel = voltValid_ ? (volt_.length()? volt_ : String("--.- V")) : String("--.- V");
  voltBox.show(voltLabel, vcol, force);

  const String ampLabel = ampValid_ ? (amp_.length()? amp_ : String("--.- A")) : String("--.- A");
  fAmp .show(ampLabel,  force);
  mapBox.show(mapCount_, mapPercent_, mapValid_, force);
  const String stateLabel = state_.length()? state_ : String("No Op");
  fStat.show(stateLabel, force);

  tft.endWrite();

  // Falls doch von außen aufgerufen wurde und Overlay aktiv ist: neu zeichnen
  if (overlay_ != OverlayKind::None) drawOverlay(overlay_);
}

// ---- Setter ----
void DashboardDisplay::setSat(SatStatus status, uint8_t used, uint8_t total){
  String label;
  switch(status){
    case SatStatus::Fix:   label="Fix";   break;
    case SatStatus::Float: label="Float"; break;
    default:               label="Inv."; break;
  }
  setSat(label, used, total);
}
void DashboardDisplay::setSat(const String& status, uint8_t used, uint8_t total){
  bool invalidLabel = status.equalsIgnoreCase("inv.") ||
                      status.equalsIgnoreCase("inval.") ||
                      status.equalsIgnoreCase("invalid");
  bool hasSatInfo = (used > 0) || (total > 0);
  bool newSatValid = !invalidLabel || hasSatInfo;

  String appliedStatus = newSatValid ? status : String("No Sat");
  uint8_t appliedUsed = newSatValid ? used : 0;
  uint8_t appliedTotal = newSatValid ? total : 0;

  satValid_ = newSatValid;
  satStatusLabel_ = appliedStatus;
  satUsed_ = appliedUsed;
  satTotal_ = appliedTotal;

  if (overlay_ == OverlayKind::None) {
    uint16_t col = satValid_ ? gfx::colorForSat(satStatusLabel_) : rgb565(140,140,140);
    satBox.show(satStatusLabel_, satUsed_, satTotal_, false, col);
  } else if (overlay_ == OverlayKind::Sat) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setSatUnavailable(){
  satValid_ = false;
  satStatusLabel_ = "No Sat";
  satUsed_ = 0;
  satTotal_ = 0;
  if (overlay_ == OverlayKind::None) {
    satBox.show(satStatusLabel_, satUsed_, satTotal_, false, rgb565(140,140,140));
  } else if (overlay_ == OverlayKind::Sat) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setRTKstr(const String& s){
  rtk_ = s;
  if (overlay_ == OverlayKind::None) {
    fRTK.show(rtk_);
  } else if (overlay_ == OverlayKind::RTK) {
    drawOverlay(overlay_);
  }
}
void DashboardDisplay::setRTK(float seconds, uint8_t decimals){
  if (!isfinite(seconds) || seconds < 0.0f || seconds >= 9999.0f) {
    rtkValid_ = false;
    setRTKstr("No RTK");
  } else {
    rtkValid_ = true;
    setRTKstr(String(seconds, decimals) + " s");
  }
}

void DashboardDisplay::setRTKUnavailable(){
  rtkValid_ = false;
  setRTKstr("No RTK");
}

void DashboardDisplay::setWifiSignal(int16_t dbm){
  wifiDbm_ = dbm;
  if (dbm <= -120 || dbm > -10) {
    wifiValid_ = false;
    wifiPercent_ = 0;
  } else {
    wifiValid_ = true;
    int16_t clamped = constrain(dbm, -100, -30);
    float ratio = (clamped + 100.0f) / 70.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    wifiPercent_ = static_cast<uint8_t>(roundf(ratio * 100.0f));
  }

  if (overlay_ == OverlayKind::None) {
    wifiBox.show(wifiValid_, wifiDbm_, wifiPercent_);
  } else if (overlay_ == OverlayKind::Wifi) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setIP(const ::IPAddress& ip){
  ipStr_ = ip.toString();
  if (overlay_ == OverlayKind::None) {
    ipBox.show(ipStr_);
  } else if (overlay_ == OverlayKind::IP) {
    drawOverlay(overlay_);
  }
}
void DashboardDisplay::setIP(const String& ipString){
  ipStr_ = ipString;
  if (overlay_ == OverlayKind::None) {
    ipBox.show(ipStr_);
  } else if (overlay_ == OverlayKind::IP) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setVoltStr(const String& s){
  volt_ = s;
  if (overlay_ == OverlayKind::None) {
    float v = isnan(voltVal_) ? Theme::kVmax : voltVal_;
    uint16_t col = voltValid_ ? gfx::colorForVolt(v) : rgb565(140,140,140);
    voltBox.show(volt_, col);
  } else if (overlay_ == OverlayKind::Volt) {
    drawOverlay(overlay_);
  }
}
void DashboardDisplay::setVolt(float volts, uint8_t decimals){
  voltVal_ = volts;
  voltValid_ = true;
  setVoltStr(String(volts, decimals) + " V");
}

void DashboardDisplay::setVoltUnavailable(){
  voltValid_ = false;
  voltVal_ = NAN;
  volt_ = "--.- V";
  if (overlay_ == OverlayKind::None) {
    voltBox.show(volt_, rgb565(140,140,140));
  } else if (overlay_ == OverlayKind::Volt) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setAmpStr(const String& s){
  amp_ = s;
  if (overlay_ == OverlayKind::None) {
    fAmp.show(amp_);
  } else if (overlay_ == OverlayKind::Amp) {
    drawOverlay(overlay_);
  }
}
void DashboardDisplay::setAmp(float amps, uint8_t decimals){
  ampValid_ = true;
  setAmpStr(String(amps, decimals) + " A");
}

void DashboardDisplay::setAmpUnavailable(){
  ampValid_ = false;
  amp_ = "--.- A";
  if (overlay_ == OverlayKind::None) {
    fAmp.show(amp_);
  } else if (overlay_ == OverlayKind::Amp) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setUltrasonicAlert(uint8_t sensorId, bool active, uint16_t distanceMm){
  unsigned long now = millis();
  switch (sensorId) {
    case 0: {
      ultrasonicLeftActive_ = active;
      ultrasonicLeftDistance_ = distanceMm;
      ultrasonicLeftUpdatedMs_ = now;
      break;
    }
    case 1: {
      ultrasonicRightActive_ = active;
      ultrasonicRightDistance_ = distanceMm;
      ultrasonicRightUpdatedMs_ = now;
      break;
    }
    default:
      return;
  }
  if (kDebugUltrasonic) {
    Serial.print("[ULTRA] RX sensor=");
    Serial.print(sensorId == 0 ? "L" : (sensorId == 1 ? "R" : "?"));
    Serial.print(" active=");
    Serial.print(active ? "true" : "false");
    Serial.print(" dist=");
    Serial.print(distanceMm);
    Serial.print("mm");
    Serial.print(" | left(ts=");
    Serial.print(ultrasonicLeftUpdatedMs_);
    Serial.print(",active=");
    Serial.print(ultrasonicLeftActive_ ? "true" : "false");
    Serial.print(") right(ts=");
    Serial.print(ultrasonicRightUpdatedMs_);
    Serial.print(",active=");
    Serial.print(ultrasonicRightActive_ ? "true" : "false");
    Serial.print(")");
    Serial.println();
  }

  bool leftFresh = ultrasonicLeftActive_ &&
                   (now - ultrasonicLeftUpdatedMs_ <= kUltrasonicHoldMs);
  bool rightFresh = ultrasonicRightActive_ &&
                    (now - ultrasonicRightUpdatedMs_ <= kUltrasonicHoldMs);
  ultrasonicLeftActive_ = leftFresh;
  ultrasonicRightActive_ = rightFresh;

  bool anyActive = leftFresh || rightFresh;
  if (anyActive) {
    String msg = "Obstacle";
    msg += " ";
    bool appended = false;
    if (leftFresh) {
      msg += "L:";
      msg += String(ultrasonicLeftDistance_);
      msg += "mm";
      appended = true;
    }
    if (rightFresh) {
      if (appended) msg += " ";
      msg += "R:";
      msg += String(ultrasonicRightDistance_);
      msg += "mm";
      appended = true;
    }
    const uint16_t color = rgb565(200,0,0);
    if (!ultrasonicMessageActive_ || msg != ultrasonicMessage_ || color != ultrasonicMessageColor_) {
      msgArea.renderTextBox(msg, color);
    }
    ultrasonicMessage_ = msg;
    ultrasonicMessageColor_ = color;
    ultrasonicMessageActive_ = true;
    ultrasonicLastUpdateMs_ = now;
    ultrasonicInactiveSinceMs_ = 0;
    ultrasonicDebugLastPrintMs_ = 0;
    ultrasonicTimeoutLogged_ = false;
    if (kDebugUltrasonic) {
      Serial.print("[ULTRA] SHOW msg='");
      Serial.print(msg);
      Serial.print("' leftFresh=");
      Serial.print(leftFresh ? "true" : "false");
      Serial.print(" rightFresh=");
      Serial.print(rightFresh ? "true" : "false");
      Serial.println();
    }
  } else {
    if (ultrasonicInactiveSinceMs_ == 0) {
      ultrasonicInactiveSinceMs_ = now;
      ultrasonicDebugLastPrintMs_ = 0;
      ultrasonicTimeoutLogged_ = false;
    }
    if (kDebugUltrasonic) {
      Serial.println("[ULTRA] Both sensors inactive; awaiting timeout");
    }
  }
}

void DashboardDisplay::setMap(uint16_t count, uint8_t percent){
  mapValid_ = true;
  mapCount_ = count; mapPercent_ = percent;
  if (overlay_ == OverlayKind::None) {
    mapBox.show(mapCount_, mapPercent_, mapValid_);
  } else if (overlay_ == OverlayKind::Map) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setMapUnavailable(){
  mapValid_ = false;
  mapCount_ = 0;
  mapPercent_ = 0;
  if (overlay_ == OverlayKind::None) {
    mapBox.show(0, 0, false);
  } else if (overlay_ == OverlayKind::Map) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::setState(const String& s){
  if (s.length() == 0 || s.equalsIgnoreCase("UNKNOWN")) {
    state_ = "No Op";
  } else {
    state_ = s;
  }
  if (overlay_ == OverlayKind::None) {
    fStat.show(state_);
  } else if (overlay_ == OverlayKind::State) {
    drawOverlay(overlay_);
  }
}

void DashboardDisplay::showBanner(){
  generalMessageValid_ = false;
  generalMessageShownMs_ = 0;
  if (!ultrasonicMessageActive_) {
    msgArea.renderBanner();
  }
}
void DashboardDisplay::showMessage(const String& msg, uint16_t color){
  generalMessage_ = msg;
  generalMessageColor_ = color;
  generalMessageValid_ = (msg.length() > 0);
  generalMessageShownMs_ = generalMessageValid_ ? millis() : 0;
  if (!ultrasonicMessageActive_) {
    if (generalMessageValid_) {
      msgArea.set(msg, color);
    } else {
      msgArea.renderBanner();
    }
  }
}

void DashboardDisplay::setMessageScrollSpeed(uint8_t pixelsPerStep, uint16_t intervalMs){
  if (pixelsPerStep < 1) pixelsPerStep = 1;
  if (intervalMs    < 5) intervalMs    = 5;
  msgArea.stepPx = pixelsPerStep;
  msgArea.stepMs = intervalMs;
}

// ========= Allgemeines Overlay =========
void DashboardDisplay::drawOverlayBox(const String& title,
                                      const String& bigLine,
                                      const String& subLine,
                                      uint16_t bigColor) {
  const int16_t W = 280, H = 160;
  const int16_t X = (Layout::ScreenW - W) / 2;
  const int16_t Y = (Layout::ScreenH - H) / 2;

  tft.fillRoundRect(X, Y, W, H, 10, TFT_WHITE);
  tft.drawRoundRect(X, Y, W, H, 10, TFT_BLACK);

  const uint16_t labelGrey = rgb565(120,120,120);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(labelGrey, TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(title, X + 10, Y + 8);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(bigColor, TFT_WHITE);
  tft.setTextSize(4);
  tft.drawString(bigLine, X + W/2, Y + H/2);

  if (subLine.length()) {
    tft.setTextSize(1);
    tft.setTextColor(labelGrey, TFT_WHITE);
    tft.setTextDatum(BC_DATUM);
    tft.drawString(subLine, X + W/2, Y + H - 8);
  }
}

void DashboardDisplay::drawOverlay(OverlayKind kind) {
  switch (kind) {
    case OverlayKind::Sat: {
      if (!satValid_) {
        drawOverlayBox("Satellites", "—", "No Sat", rgb565(140,140,140));
      } else {
        String counts = String(satUsed_) + " / " + String(satTotal_);
        String stat   = satStatusLabel_.length()? satStatusLabel_ : String("Fix");
        uint16_t col  = gfx::colorForSat(stat);
        drawOverlayBox("Satellites", counts, stat, col);
      }
      break;
    }
    case OverlayKind::RTK: {
      if (!rtkValid_) {
        drawOverlayBox("RTK Age", "—", "No RTK", rgb565(140,140,140));
      } else {
        String s = rtk_.length()? rtk_ : "0.0 s";
        drawOverlayBox("RTK Age", s, "");
      }
      break;
    }
    case OverlayKind::Wifi: {
      if (!wifiValid_) {
        drawOverlayBox("WiFi Signal", "No link", "", rgb565(200, 0, 0));
      } else {
        String big = String(wifiPercent_) + "%";
        String sub = String(wifiDbm_) + " dBm";
        uint16_t col = gfx::colorForSignal(wifiPercent_);
        drawOverlayBox("WiFi Signal", big, sub, col);
      }
      break;
    }
    case OverlayKind::IP: {
      String ip = ipStr_.length()? ipStr_ : "No IP";
      drawOverlayBox("IP Address", ip, "");
      break;
    }
    case OverlayKind::Volt: {
      if (!voltValid_) {
        drawOverlayBox("Battery", "--.- V", "", rgb565(140,140,140));
      } else {
        float v = isnan(voltVal_) ? Theme::kVmax : voltVal_;
        uint16_t vcol = gfx::colorForVolt(v);
        String s = volt_.length()? volt_ : String("29.2 V");
        drawOverlayBox("Battery", s, "", vcol);
      }
      break;
    }
    case OverlayKind::Amp: {
      if (!ampValid_) {
        drawOverlayBox("Current", "--.- A", "", rgb565(140,140,140));
      } else {
        String s = amp_.length()? amp_ : "0.0 A";
        drawOverlayBox("Current", s, "");
      }
      break;
    }
    case OverlayKind::Map: {
      if (!mapValid_) {
        drawOverlayBox("Map progress", "—", "No Map", rgb565(140,140,140));
      } else {
        String big = String(mapPercent_) + "%";
        String sub = String("Map: ") + gfx::fmtThousands(mapCount_);
        uint16_t col = (mapPercent_ >= 100) ? rgb565(0,110,0) : TFT_BLACK;
        drawOverlayBox("Map progress", big, sub, col);
      }
      break;
    }
    case OverlayKind::State: {
      String s = state_.length()? state_ : "No Op";
      drawOverlayBox("State", s, "");
      break;
    }
    default: break;
  }
}



void DashboardDisplay::showOverlay(OverlayKind kind, bool show) {
  if (show) {
    overlay_ = kind;
    drawOverlay(overlay_);
  } else {
    if (overlay_ != OverlayKind::None) {
      overlay_ = OverlayKind::None;
      forceRefresh(); // Voll-Refresh, damit keine Reste bleiben
    }
  }
}

// Kompatibilität: alt -> neu
void DashboardDisplay::drawSatDetailsOverlay() { drawOverlay(OverlayKind::Sat); }
void DashboardDisplay::showSatDetails(bool show) { showOverlay(OverlayKind::Sat, show); }
void DashboardDisplay::toggleSatDetails() {
  if (overlay_ == OverlayKind::Sat) showOverlay(OverlayKind::Sat, false);
  else                              showOverlay(OverlayKind::Sat, true);
}

// Touch: Overlay schließt bei JEDEM Tap; sonst Felder öffnen
bool DashboardDisplay::onTouch(int16_t x, int16_t y) {
  if (overlay_ != OverlayKind::None) { showOverlay(overlay_, false); return true; }

  const int16_t Lx = Layout::L, Rx = Layout::R;
  const int16_t W  = Layout::boxW, H = Layout::boxH;

  auto hit = [&](int16_t bx, int16_t by)->bool{
    return (x >= bx && x < bx+W && y >= by && y < by+H);
  };

  if (hit(Lx, Layout::Y0)) { showOverlay(OverlayKind::Sat,  true); return true; }
  if (hit(Lx, Layout::Y1)) { showOverlay(OverlayKind::RTK,  true); return true; }
  if (hit(Lx, Layout::Y2)) { showOverlay(OverlayKind::Volt, true); return true; }
  if (hit(Lx, Layout::Y3)) { showOverlay(OverlayKind::Amp,  true); return true; }

  if (hit(Rx, Layout::Y0)) { showOverlay(OverlayKind::Wifi, true); return true; }
  if (hit(Rx, Layout::Y1)) { showOverlay(OverlayKind::IP,   true); return true; }
  if (hit(Rx, Layout::Y2)) { showOverlay(OverlayKind::Map,  true); return true; }
  if (hit(Rx, Layout::Y3)) { showOverlay(OverlayKind::State,true); return true; }

  return false;
}
