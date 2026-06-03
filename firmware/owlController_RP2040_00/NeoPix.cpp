#include "config.h"
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include "NeoPix.h"

Adafruit_NeoPixel strip(LED_COUNT, NeoPix_Pin, NEO_GRB + NEO_KHZ800);

namespace {
struct NeoPixColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

inline NeoPixColor getModuleColorPreset(uint8_t id) {
  switch (id) {
    case 0: return {125, 125, 125};
    case 1: return {125, 0, 0};
    case 2: return {0, 125, 0};
    case 3: return {0, 0, 125};
    case 4: return {125, 125, 0};
    case 5: return {0, 125, 125};
    case 6: return {125, 0, 125};
    case 7: return {125, 60, 0};
    default: return {125, 125, 125};
  }
}

inline uint8_t scaleChannel(uint8_t base, float scale) {
  int v = (int)(base * scale);
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

inline uint8_t clampChannel(int v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)v;
}

inline void setStripPixel(int ledNo, int r, int g, int b, bool flush) {
  if (ledNo < 0 || ledNo >= LED_COUNT) return;
  strip.setPixelColor((uint16_t)ledNo, strip.Color(clampChannel(r), clampChannel(g), clampChannel(b)));
  if (flush) strip.show();
}

inline void fillStrip(int r, int g, int b, bool flush) {
  strip.fill(strip.Color(clampChannel(r), clampChannel(g), clampChannel(b)), 0, LED_COUNT);
  if (flush) strip.show();
}

inline void clearStrip(bool flush) {
  strip.clear();
  if (flush) strip.show();
}
}  // namespace

NeoPix::NeoPix(){
  i = 0;
  old_scene = -1;
  runtimeMode = LED_MODE_IDLE;
  runtimeAnimId = 0;
  runtimeColorId = 0;
  runtimeStartLed = 0;
  runtimeLedCount = 0;
  runtimeBrightnessScale = 1.0f;
  runtimeLastMs = 0;
  runtimePhase = 0;
}

void NeoPix::begin()
{
  strip.begin();
  strip.show();
  fillStrip(3,0,0, true);
  delay(500);
  NeoPixel_scene(11,1);
  NeoPixel_scene(12,1);
  NeoPixel_scene(11,1);
  clearStrip(true);
}

void NeoPix::setNeoPixel(int ledNo,int r,int g,int b){
  runtimeMode = LED_MODE_IDLE;
  setStripPixel(ledNo,r,g,b,true);
  delay(1);
}

void NeoPix::NeoPixel_scene(int sceneNo,float Pix_brightness ){
  if (Pix_brightness<=0.01 || Pix_brightness>2) Pix_brightness=0.01;
  if (Pix_brightness>2) Pix_brightness=2;
  if(old_scene != sceneNo){
    runtimeMode = LED_MODE_IDLE;
    switch (sceneNo) {
      case 0:
        fillStrip(0,0,0, true);
        break;
      case 1:
        fillStrip(int(125*Pix_brightness),0,0, true);
        break;
      case 2:
        fillStrip(0,int(125*Pix_brightness),0, true);
        break;
      case 3:
        fillStrip(0,0,int(125*Pix_brightness), true);
        break;
      case 4:
        fillStrip(int(125*Pix_brightness),int(125*Pix_brightness),int(125*Pix_brightness), true);
        break;
      case 5:
        fillStrip(int(125*Pix_brightness),int(15*Pix_brightness),int(15*Pix_brightness), true);
        break;
      case 6:
        fillStrip(0,int(125*Pix_brightness),0, false);
        if (LED_COUNT == 144){
          for(i=0;i<=56;i++){
            setStripPixel((LED_COUNT/2-28)+i,int(125*Pix_brightness*0.5),0,0,true);
            delayMicroseconds(750);
          }
        } else {
          fillStrip(int(125*Pix_brightness),int(125*Pix_brightness),0, true);
        }
        break;
      case 7:
        if (LED_COUNT == 144){
          fillStrip(0,int(125*Pix_brightness),0, false);
          for(i=0;i<=28;i++){
            setStripPixel(i,int(125*Pix_brightness*0.5),0,0,true);
            setStripPixel(116+i,int(125*Pix_brightness*0.5),0,0,true);
            delayMicroseconds(750);
          }
        } else {
          fillStrip(int(125*Pix_brightness),0,int(125*Pix_brightness), true);
        }
        break;
      case 10:
        for(i=0;i<LED_COUNT;i=i+3){
          setStripPixel(i,int(125*Pix_brightness),0,0,false);
          if (i + 1 < LED_COUNT) setStripPixel(i+1,int(125*Pix_brightness),0,0,false);
          if (i + 2 < LED_COUNT) setStripPixel(i+2,int(125*Pix_brightness),0,0,false);
        }
        setStripPixel(LED_COUNT - 1,0,0,0,true);
        break;
      case 11:
        clearStrip(true);
        for(i=0;i<LED_COUNT/2;i++){
          if (i >= 2) setStripPixel(i-2,0,0,0,false);
          setStripPixel(i,0,0,125*Pix_brightness,true);
          if ((LED_COUNT - i + 2) < LED_COUNT) setStripPixel(LED_COUNT-i+2,0,0,0,false);
          if ((LED_COUNT - i) < LED_COUNT) setStripPixel(LED_COUNT-i,0,0,125*Pix_brightness,true);
          delay(1);
        }
        break;
      case 12:
        clearStrip(true);
        for(i=LED_COUNT/2;i>0;i--){
          if (i >= 2) setStripPixel(i-2,0,0,0,false);
          setStripPixel(i,0,0,125*Pix_brightness,true);
          if ((LED_COUNT - i + 2) < LED_COUNT) setStripPixel(LED_COUNT-i+2,0,0,0,false);
          if ((LED_COUNT - i) < LED_COUNT) setStripPixel(LED_COUNT-i,0,0,125*Pix_brightness,true);
          delay(1);
        }
        break;
      default:
        fillStrip(0,0,0, true);
        break;
    }
    old_scene = sceneNo;
  }
}

bool NeoPix::clampSegment(uint8_t startLed, uint8_t ledCount, int &startOut, int &endOut){
  if (LED_COUNT <= 0) return false;
  if (startLed >= LED_COUNT) return false;
  if (ledCount == 0) return false;
  startOut = (int)startLed;
  int rawEnd = startOut + (int)ledCount;
  if (rawEnd > LED_COUNT) rawEnd = LED_COUNT;
  endOut = rawEnd;
  return (startOut < endOut);
}

float NeoPix::clampBrightnessScale(uint8_t brightness) const{
  int b = (int)brightness;
  if (b < 0) b = 0;
  if (b > 200) b = 200;
  return ((float)b) / 100.0f;
}

void NeoPix::applySolidInternal(int startLed, int endExclusive, uint8_t colorId, float brightnessScale){
  NeoPixColor base = getModuleColorPreset(colorId);
  for (int idx = startLed; idx < endExclusive; ++idx){
    const bool flush = (idx == (endExclusive - 1));
    setStripPixel(idx,
                           scaleChannel(base.r, brightnessScale),
                           scaleChannel(base.g, brightnessScale),
                           scaleChannel(base.b, brightnessScale),
                           flush);
  }
}

void NeoPix::clearSegmentInternal(int startLed, int endExclusive){
  for (int idx = startLed; idx < endExclusive; ++idx){
    const bool flush = (idx == (endExclusive - 1));
    setStripPixel(idx, 0, 0, 0, flush);
  }
}

void NeoPix::solidSegment(uint8_t startLed, uint8_t ledCount, uint8_t colorId, uint8_t brightness){
  int start = 0;
  int end = 0;
  if (!clampSegment(startLed, ledCount, start, end)) return;
  runtimeMode = LED_MODE_IDLE;
  old_scene = -1;
  applySolidInternal(start, end, colorId, clampBrightnessScale(brightness));
}

void NeoPix::startAnimSingle(uint8_t animId, uint8_t ledCount, uint8_t colorId, uint8_t brightness){
  int start = 0;
  int end = 0;
  if (!clampSegment(0, ledCount, start, end)) return;
  runtimeMode = LED_MODE_ANIM_SINGLE;
  runtimeAnimId = animId;
  runtimeColorId = colorId;
  runtimeStartLed = (uint8_t)start;
  runtimeLedCount = (uint8_t)(end - start);
  runtimeBrightnessScale = clampBrightnessScale(brightness);
  runtimePhase = 0;
  runtimeLastMs = 0;
  old_scene = -1;
  renderAnimSingle();
}

void NeoPix::startAnimMulti(uint8_t animId, uint8_t ledCount, uint8_t brightness){
  int start = 0;
  int end = 0;
  if (!clampSegment(0, ledCount, start, end)) return;
  runtimeMode = LED_MODE_ANIM_MULTI;
  runtimeAnimId = animId;
  runtimeColorId = 0;
  runtimeStartLed = (uint8_t)start;
  runtimeLedCount = (uint8_t)(end - start);
  runtimeBrightnessScale = clampBrightnessScale(brightness);
  runtimePhase = 0;
  runtimeLastMs = 0;
  old_scene = -1;
  renderAnimMulti();
}

void NeoPix::offAll(){
  runtimeMode = LED_MODE_IDLE;
  old_scene = 0;
  fillStrip(0, 0, 0, true);
}

void NeoPix::offSegment(uint8_t startLed, uint8_t ledCount){
  int start = 0;
  int end = 0;
  if (!clampSegment(startLed, ledCount, start, end)) return;
  if (runtimeMode != LED_MODE_IDLE){
    int aStart = (int)runtimeStartLed;
    int aEnd = aStart + (int)runtimeLedCount;
    if (!(end <= aStart || start >= aEnd)){
      runtimeMode = LED_MODE_IDLE;
    }
  }
  old_scene = -1;
  clearSegmentInternal(start, end);
}

void NeoPix::tick(){
  if (runtimeMode == LED_MODE_ANIM_SINGLE){
    renderAnimSingle();
  } else if (runtimeMode == LED_MODE_ANIM_MULTI){
    renderAnimMulti();
  }
}

void NeoPix::renderAnimSingle(){
  int start = (int)runtimeStartLed;
  int end = start + (int)runtimeLedCount;
  if (start < 0) start = 0;
  if (end > LED_COUNT) end = LED_COUNT;
  if (start >= end) return;

  const unsigned long now = millis();
  const unsigned long intervalMs = 50;
  if (runtimeLastMs != 0 && (now - runtimeLastMs) < intervalMs) return;
  runtimeLastMs = now;
  runtimePhase++;

  NeoPixColor base = getModuleColorPreset(runtimeColorId);

  if (runtimeAnimId == 0){
    float phase = (float)(runtimePhase % 24) / 24.0f;
    float level = 0.15f + 0.85f * (0.5f + 0.5f * sinf(phase * 2.0f * 3.14159f));
    for (int idx = start; idx < end; ++idx){
      const bool flush = (idx == (end - 1));
      setStripPixel(idx,
                             scaleChannel(base.r, runtimeBrightnessScale * level),
                             scaleChannel(base.g, runtimeBrightnessScale * level),
                             scaleChannel(base.b, runtimeBrightnessScale * level),
                             flush);
    }
    return;
  }

  if (runtimeAnimId == 1){
    int len = end - start;
    int head = start + (runtimePhase % (uint16_t)len);
    for (int idx = start; idx < end; ++idx){
      float level = 0.08f;
      int dist = head - idx;
      if (dist < 0) dist = -dist;
      if (dist == 0) level = 1.0f;
      else if (dist == 1) level = 0.4f;
      const bool flush = (idx == (end - 1));
      setStripPixel(idx,
                             scaleChannel(base.r, runtimeBrightnessScale * level),
                             scaleChannel(base.g, runtimeBrightnessScale * level),
                             scaleChannel(base.b, runtimeBrightnessScale * level),
                             flush);
    }
    return;
  }

  int len = end - start;
  int fillCount = (runtimePhase % (uint16_t)(len + 1));
  for (int p = 0; p < len; ++p){
    int idx = start + p;
    bool on = (p < fillCount);
    const bool flush = (idx == (end - 1));
    setStripPixel(idx,
                           on ? scaleChannel(base.r, runtimeBrightnessScale) : 0,
                           on ? scaleChannel(base.g, runtimeBrightnessScale) : 0,
                           on ? scaleChannel(base.b, runtimeBrightnessScale) : 0,
                           flush);
  }
}

void NeoPix::renderAnimMulti(){
  int start = (int)runtimeStartLed;
  int end = start + (int)runtimeLedCount;
  if (start < 0) start = 0;
  if (end > LED_COUNT) end = LED_COUNT;
  if (start >= end) return;

  const unsigned long now = millis();
  const unsigned long intervalMs = 50;
  if (runtimeLastMs != 0 && (now - runtimeLastMs) < intervalMs) return;
  runtimeLastMs = now;
  runtimePhase++;

  int len = end - start;
  if (runtimeAnimId == 0){
    for (int p = 0; p < len; ++p){
      int idx = start + p;
      uint8_t phase = (uint8_t)((p * 256 / (len > 0 ? len : 1) + runtimePhase * 5) & 0xFF);
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
      if (phase < 85){
        r = (uint8_t)(phase * 3);
        g = (uint8_t)(255 - phase * 3);
      } else if (phase < 170){
        phase -= 85;
        g = (uint8_t)(phase * 3);
        b = (uint8_t)(255 - phase * 3);
      } else {
        phase -= 170;
        b = (uint8_t)(phase * 3);
        r = (uint8_t)(255 - phase * 3);
      }
      const bool flush = (idx == (end - 1));
      setStripPixel(idx,
                             scaleChannel(r, runtimeBrightnessScale),
                             scaleChannel(g, runtimeBrightnessScale),
                             scaleChannel(b, runtimeBrightnessScale),
                             flush);
    }
    return;
  }

  if (runtimeAnimId == 1){
    for (int p = 0; p < len; ++p){
      int idx = start + p;
      bool on = (((p + runtimePhase) % 3) == 0);
      const bool flush = (idx == (end - 1));
      setStripPixel(idx,
                             on ? scaleChannel(125, runtimeBrightnessScale) : 0,
                             on ? scaleChannel(125, runtimeBrightnessScale) : 0,
                             on ? scaleChannel(125, runtimeBrightnessScale) : 0,
                             flush);
    }
    return;
  }

  bool phaseA = ((runtimePhase / 4) % 2) == 0;
  for (int p = 0; p < len; ++p){
    int idx = start + p;
    bool firstHalf = (p < (len / 2));
    bool redOn = phaseA ? firstHalf : !firstHalf;
    bool blueOn = !redOn;
    const bool flush = (idx == (end - 1));
    setStripPixel(idx,
                           redOn ? scaleChannel(125, runtimeBrightnessScale) : 0,
                           0,
                           blueOn ? scaleChannel(125, runtimeBrightnessScale) : 0,
                           flush);
  }
}
