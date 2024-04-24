//#include <Adafruit_NeoPixel.h>
#include "config.h"
#include <NeoPixelConnect.h>
#include "NeoPix.h"

//Adafruit_NeoPixel strip(LED_COUNT, NeoPix_Pin, NEO_GRBW + NEO_KHZ800);
NeoPixelConnect strip(25, 144);


NeoPix::NeoPix(){}
 void NeoPix::begin()
   {
    strip.neoPixelFill(3,0,0, true);
    delay(500);
    NeoPixel_scene(11,1);
    NeoPixel_scene(12,1);
    NeoPixel_scene(11,1);
   strip.neoPixelClear(true);  
  }
   
void NeoPix::setNeoPixel(int ledNo,int r,int g,int b){
   strip.neoPixelSetValue(ledNo,r,g,b,true);
   delay(1);  
}
void NeoPix::NeoPixel_scene(int sceneNo,float Pix_brightness ){
   if (Pix_brightness<=0.01 ||Pix_brightness>2) Pix_brightness=0.01;
   if (Pix_brightness>2) Pix_brightness=2;
   if(old_scene != sceneNo){
   switch (sceneNo) {
    case 0:   // clear strip, all pix are off
      strip.neoPixelFill(0,0,0, true);
    break;
    case 1:   // clear strip, all pix are red
      strip.neoPixelFill(int(125*Pix_brightness),0,0, true);
    break;
    case 2:   // clear strip, all pix are greeb
      strip.neoPixelFill(0,int(125*Pix_brightness),0, true);
    break;
    case 3:   // clear strip, all pix are blue
      strip.neoPixelFill(0,0,int(125*Pix_brightness), true);
    break;   
    case 4:   // clear strip, all pix are white
      strip.neoPixelFill(int(125*Pix_brightness),int(125*Pix_brightness),int(125*Pix_brightness), true);
    break; 
    case 5:   // clear strip, all pix are pink
      strip.neoPixelFill(int(125*Pix_brightness),int(15*Pix_brightness),int(15*Pix_brightness), true);
    break;
    case 6:   // clear strip, all pix are pink
      strip.neoPixelFill(0,int(125*Pix_brightness),0, false);
      if (LED_COUNT ==144){
        for(i=0;i<=56;i++){
          strip.neoPixelSetValue((LED_COUNT/2-28)+i,int(125*Pix_brightness*0.5),0,0,true); 
          delayMicroseconds (750);
        }
      }
       else strip.neoPixelFill(int(125*Pix_brightness),int(125*Pix_brightness),0, true); 
     Serial.println("Blinker links"); 
    break; 
    case 7:   // 
    if (LED_COUNT ==144){
      strip.neoPixelFill(0,int(125*Pix_brightness),0, false);
      for(i=0;i<=28;i++){
        strip.neoPixelSetValue(i,int(125*Pix_brightness*0.5),0,0,true);
        strip.neoPixelSetValue(116+i,int(125*Pix_brightness*0.5),0,0,true);
        delayMicroseconds (750);
       }
      }
     else strip.neoPixelFill(int(125*Pix_brightness),0,int(125*Pix_brightness), true);  
      
      Serial.println("Blinker rechts");
    break; 
    case 10:   // clear strip, all pix are red green blue LEDs runtime ~4ms with 70 NeoPixel
    for(i=0;i<LED_COUNT;i=i+3){
      strip.neoPixelSetValue(i,int(125*Pix_brightness),0,0,false);
      strip.neoPixelSetValue(i+1,int(125*Pix_brightness),0,0,false);
      strip.neoPixelSetValue(i+2,int(125*Pix_brightness),0,0,false);
    }  
    strip.neoPixelSetValue(LED_COUNT,0,0,0,true);
    break;
    case 11: // use only during initialisation, runtime ~1,3s with 70 NeoPixel
    strip.neoPixelClear(true);
    for(i=0;i<LED_COUNT/2;i++){
      strip.neoPixelSetValue(i-2,0,0,0,false);
      strip.neoPixelSetValue(i,0,0,125*Pix_brightness,true);
      strip.neoPixelSetValue(LED_COUNT-i+2,0,0,0,false);
      strip.neoPixelSetValue(LED_COUNT-i,0,0,125*Pix_brightness,true);      
      delay (1);
    }
    break;
    case 12: // use only during initialisation, runtime ~1,3s with 70 NeoPixel
    strip.neoPixelClear(true);
    for(i=LED_COUNT/2;i>0;i--){
      strip.neoPixelSetValue(i-2,0,0,0,false);
      strip.neoPixelSetValue(i,0,0,125*Pix_brightness,true);
      strip.neoPixelSetValue(LED_COUNT-i+2,0,0,0,false);
      strip.neoPixelSetValue(LED_COUNT-i,0,0,125*Pix_brightness,true);
      delay (1);
    } 
    break;    
    default:
     strip.neoPixelFill(0,0,0, true);
    break;
   }
   old_scene = sceneNo;
 } 
}
