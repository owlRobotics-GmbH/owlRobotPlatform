
#include <Arduino.h>
#include <Adafruit_ILI9341.h>  //  >1.4" TFT
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "oledDisp.h"
#include "Funkt.h"
#include <TCA9548A.h>
#include "config.h"
#include "owlcontrol.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C 


extern Funkt myF;
extern TCA9548A I2CMux;

 Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


oledDisp::oledDisp() : raspberryPiIP("No IP") {}

void oledDisp::setIP(const String &ip) {
  raspberryPiIP = ip;
}


void oledDisp::begin(){
  I2CMux.openChannel(oled_I2CMuxChn);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(5, 5);
  display.print("Setup");
  display.display();
}  

void oledDisp::oledPowerOff(){
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(2, 0);
  display.print("Shut down");
  display.setCursor(2, 18);
  display.print("Power off");
  display.display();
}

void oledDisp::status(){
  I2CMux.openChannel(oled_I2CMuxChn);
  float volt= myF.ADC_read(7)/1000*34.0;
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.fillRect(0,3,5,5,WHITE); 
  display.fillRect(3,1,15,10,WHITE);
  display.setCursor(30, 1); // (30, 5)
  if( volt>5) {
    display.setCursor(45, 1); // (45, 5)
    display.print(volt,1);  // to be adjusted 
    display.print("V");
   } else  display.print("USB");
  display.setCursor(110, 1);
   if (myF.rain())display.print(char(0xB0));
     else display.print(char(0x0F));
  

  // show Raspberry Pi IP address
  display.setTextColor(WHITE);
  //String raspberryPiIP = "192.168.100.100"; // Beispiel-IP-Adresse
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("IP: ");
  display.print(raspberryPiIP);
  display.setTextSize(2);

  display.display();
}  
