/// act. software version 
#define VERS "HermesPico,1.0.0"

/*     Motor Parameter         */

// Invert / set initial Motor direction 
#define initMotorDir_left  0
#define initMotorDir_right 1

// Rain sensor
// value ~18mV open contact, ~ 750mV - 1500mV water bridge, ~2500mV short cut
// rainDetect < 25 rain detection is allwasy on, rainDetect > 1800 rain detection is allwasy off
#define rainDetect 1250 

//ADC
#define ADC_Resulution 10 //bit


#define PCF8591_default  0x48  // default
#define PCF8591_modified 0x4A

//OLED Display, define I²C MUX Port
#define oled_I2CMuxChn 0

//RGB LED WS2812 Neopixel
#define LED_COUNT  144
