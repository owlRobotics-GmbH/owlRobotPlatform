#include "config.h"

#pragma once

enum NeoPixScene : uint8_t {
  NEOPIX_SCENE_OFF = 0,
  NEOPIX_SCENE_RED = 1,
  NEOPIX_SCENE_GREEN = 2,
  NEOPIX_SCENE_BLUE = 3,
  NEOPIX_SCENE_WHITE = 4,
  NEOPIX_SCENE_PINK = 5,
  NEOPIX_SCENE_BLINK_LEFT = 6,
  NEOPIX_SCENE_BLINK_RIGHT = 7,
  NEOPIX_SCENE_RGB_TEST = 10,
  NEOPIX_SCENE_INIT_WIPE_FWD = 11,
  NEOPIX_SCENE_INIT_WIPE_REV = 12,
};

#ifndef NeoPix_Pin
#define NeoPix_Pin 25
#endif

class NeoPix {
  public:
   NeoPix();
   void begin();
   void status();
   void setNeoPixel(int ledNo,int r,int g,int b);
   void NeoPixel_scene(int sceneNo,float Pix_brightness);
   void solidSegment(uint8_t startLed, uint8_t ledCount, uint8_t colorId, uint8_t brightness);
   void startAnimSingle(uint8_t animId, uint8_t ledCount, uint8_t colorId, uint8_t brightness);
   void startAnimMulti(uint8_t animId, uint8_t ledCount, uint8_t brightness);
   void offAll();
   void offSegment(uint8_t startLed, uint8_t ledCount);
   void tick();
   int scene_default=0;
   float default_brightness=1;
  private:
   enum LedRuntimeMode : uint8_t {
     LED_MODE_IDLE = 0,
     LED_MODE_ANIM_SINGLE = 1,
     LED_MODE_ANIM_MULTI = 2,
   };
   bool clampSegment(uint8_t startLed, uint8_t ledCount, int &startOut, int &endOut);
   float clampBrightnessScale(uint8_t brightness) const;
   void applySolidInternal(int startLed, int endExclusive, uint8_t colorId, float brightnessScale);
   void clearSegmentInternal(int startLed, int endExclusive);
   void renderAnimSingle();
   void renderAnimMulti();
   int i;
   int old_scene;
   LedRuntimeMode runtimeMode;
   uint8_t runtimeAnimId;
   uint8_t runtimeColorId;
   uint8_t runtimeStartLed;
   uint8_t runtimeLedCount;
   float runtimeBrightnessScale;
   unsigned long runtimeLastMs;
   uint16_t runtimePhase;
};
