#include "config.h"
#define t_CS 12                
#define t_IRQ 13 
#define TFT_CS 27 
#define TFT_RST 14
#define TFT_DC 05
#define TFT_MOSI 23
#define TFT_CLK 18
#define TFT_MISO 19
#define X_0 170
#define Y_0 130
#define X_0 170
#define Y_0 130

class oledDisp {
  public:
   oledDisp(); 
   void begin();
   void status();
   void oledPowerOff();

   void setIP(const String &ip);
  private:
   String raspberryPiIP; // Raspberry Pi IP address to show on display
};
