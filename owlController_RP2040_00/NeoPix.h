#include "config.h"

#define NeoPix_Pin 25

class NeoPix {
  public:
   NeoPix(); 
   void begin();
   void status();
   void setNeoPixel(int ledNo,int r,int g,int b);
   void NeoPixel_scene(int sceneNo,float Pix_brightness  );
   int scene_default=0;
   float default_brightness=1;
  private:
   int i;
   int old_scene;
};
