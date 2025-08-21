
#define PIEPER_Pin 2
#define LED_Pin 3
#define ADC0 A0
//#define ADCadr 0x68
#define ADC_Ref 3000
#define PowerHold_Pin 11
#define anaMUX_Adr0 13
#define anaMUX_Adr1 14 
#define anaMUX_Adr2 15 
#define LoadPowerPWM_Pin 23
#define CANioPower_Pin 06

#define PIon_off 10

#define out1 22
#define out2 23
#define out3 24
#define out4 25

#define in1 22

#define NeoPix_Pin 25

#pragma once


class Funkt {
  public:
   Funkt();          // initialized with no parameter
   void begin();
   void pieper(int ToneTime); 
  // void Relais(int RelNr, bool stat);
  void NeoPixel(int ledNo,int r,int g,int b,int bright);
   void extPieper(bool on_off);
   void anaMux(byte chn_select);
   void refreshIOports();
   int IOext_readReg(byte adr, byte reg); 
   void IOext_writeReg(byte adr, byte reg, byte val_0,byte val_1); 
 //  void ADC_init();
   float ADC_read(int muxChnl);
   bool rain();
   float getBatteryVoltage();
   float getChargerVoltage();   
//   float read_M_Current(int motor);
   void PowerHold(bool holdPWR);
   void PIpwr(bool on_off);
   void CAN_Power(bool on_off);
   void readIN_port();
   void LoadPowerPWM(int perct);
   bool SW_Power_off();
   byte IOPort0,IOPort1,IOPort21_P0,IOPort21_P1;
   int  IOPort;
   int  INport; 
   bool IN_Pin[5];
   bool OUT_Pin[5];
   int  i;
   float mVrain;
  private:  
   int val, port,InPort;
   int ADC_Value;
   float currOffset, batteryVoltage, chargerVoltage;
};
