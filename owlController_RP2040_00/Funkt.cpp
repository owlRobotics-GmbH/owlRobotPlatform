#include <Arduino.h>
#include <TCA9548A.h>
#include "Funkt.h"
//#include "MotorContr.h"
#include <MCP342x.h>
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <RP2040_PWM.h>

//#define DEBUG 1


//extern MotorContr myMCtr;
//MCP342x adc = MCP342x(ADCadr);
 
TCA9548A I2CMux;
RP2040_PWM* PWM_load;
Adafruit_NeoPixel strip2(LED_COUNT, NeoPix_Pin, NEO_GRBW + NEO_KHZ800);

 Funkt::Funkt(){}
 void Funkt::begin()
   { 
    pinMode(PIon_off,OUTPUT);  //PIN 10
    pinMode(PIEPER_Pin,OUTPUT);  //PIN 2
    pinMode(PowerHold_Pin,OUTPUT);  //Pico 11  
    pinMode(CANioPower_Pin,OUTPUT);  //Pico 23
    pinMode(anaMUX_Adr0,OUTPUT);
    pinMode(anaMUX_Adr1,OUTPUT);
    pinMode(anaMUX_Adr2,OUTPUT);
    digitalWrite (PIon_off,0);    // HIGH = PI Power on 
    digitalWrite (anaMUX_Adr0,0);
    digitalWrite (anaMUX_Adr1,0);
    digitalWrite (anaMUX_Adr2,0);
    digitalWrite (LoadPowerPWM_Pin,0);
    digitalWrite (CANioPower_Pin,0);
   // Wire.setPins(6,7);
    I2CMux.begin(Wire);
    delay(2);

    // init PCA9555 for IO Ports IN  ports 1.0-1.3  OUT; Ports 0.0-0.3
    I2CMux.openChannel(6);   // switch I2C Adr.
    IOext_writeReg(0x20,0x6,0b11110000,0b11111111);  //Init Port 0 bit 0-3 putput Port 1 as input
    IOext_writeReg(0x20,0x2,0b00001111,0b00000000);  // Set all outputs to 0

    PowerHold(1);
    extPieper(0);   
    strip2.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip2.show();            // Turn OFF all pixels ASAP
    strip2.setPixelColor(0,  strip2.Color(0, 75, 0));
    delay(5);
    strip2.setPixelColor(1,  strip2.Color(0, 160, 0));
    strip2.setBrightness(24);
    strip2.show();

    // Init PWM Chan. to controll Akku load power
    PWM_load = new RP2040_PWM(LoadPowerPWM_Pin, 100, 0);
    PWM_load->setPWM(LoadPowerPWM_Pin, 100, 0);
   }  

   
void Funkt::NeoPixel(int ledNo,int r,int g,int b,int bright){
    strip2.setPixelColor(ledNo,strip2.Color(r, g, b));
    strip2.setBrightness(bright);
    strip2.show();  
}


float Funkt::ADC_read(int muxChnl){
    anaMux(muxChnl);  // analog mux port 
    delay(2);
    ADC_Value = analogRead(A0);
    float mVolt = ADC_Value* ADC_Ref/pow(2,ADC_Resulution);
    #ifdef DEBUG
      Serial.print("  ADC: ");Serial.print(ADC_Value);
      Serial.print("  mV: ");Serial.println(mVolt);
    #endif
    return (mVolt);            
}

bool Funkt::rain(){
     mVrain = ADC_read(3);
    #ifdef DEBUG
      Serial.print("  rain: ");Serial.print(mVrain);Serial.println("  mV: ");
    #endif
    return (mVrain>rainDetect);            
}

float Funkt::getBatteryVoltage(){
  float adcVolt = ADC_read(7);  // returns millivolt
  batteryVoltage = adcVolt/10.0 * 340.0 / 1000.0;  // 3v ADC input equals 100v sensor value  
    #ifdef DEBUG
      Serial.print("  batV: ");Serial.print(batteryVoltage);Serial.println("  mV ");
    #endif
  return batteryVoltage; // returns millivolt
}

float Funkt::getChargerVoltage(){  
  float adcVolt = ADC_read(0); // returns millivolt
  chargerVoltage = adcVolt/10.0 * 100.0 / 1000.0;  // 3v ADC input equals 30V sensor value
    #ifdef DEBUG
      Serial.print("  chgV: ");Serial.print(chargerVoltage);Serial.println("  mV ");
    #endif
  return chargerVoltage; // returns millivolt
}


void Funkt::LoadPowerPWM(int perct){
  PWM_load->setPWM(LoadPowerPWM_Pin, 100, perct);
}

 
void Funkt::PowerHold(bool holdPWR){        //ok
  digitalWrite(PowerHold_Pin,holdPWR);
 }
 
bool Funkt::SW_Power_off(){                 //ok
     digitalWrite (PowerHold_Pin,0);        // Power hold pin 11, to power off = 0
     return(0);     
 }  
void Funkt::CAN_Power(bool on_off){         //ok
     digitalWrite (CANioPower_Pin,on_off);  //  pin 06, to power off = 0     
 }  
void Funkt::PIpwr (bool val){
    digitalWrite (PIon_off,val);
  }   

void Funkt::extPieper(bool on_off)
   { 
    digitalWrite(PIEPER_Pin,on_off);
   }
    

  // chn_select :  
  //    110         anaMUX_Adr0 : 0
  //                anaMUX_Adr1 : 1
  //                anaMUX_Adr2 : 1

  void Funkt::anaMux(byte chn_select)
   {
     digitalWrite(anaMUX_Adr0,chn_select & 0b00000001);
     digitalWrite(anaMUX_Adr1,(chn_select>>1) & 0b00000001);
     digitalWrite(anaMUX_Adr2,(chn_select>>2) & 0b00000001);
     delay(2);
   }


 //PCA9555 Function 
   void Funkt::refreshIOports(){
    byte ioport=0;
      for (i=1;i<5;i++){    
        ioport= ioport +(OUT_Pin[i]*pow(2,i-1));
      }      
      IOext_writeReg(0x20,0x2,ioport,ioport);
      ioport = IOext_readReg(0x20,1);
  //    Serial.print("IN Pin ");
      for (i=1;i<5;i++){    
          IN_Pin[i]=ioport&0x01<<(i-1);
  //        Serial.print(IN_Pin[i]);
      }    
  //    Serial.println(" ");
   }  
/*
  int Funkt::readIn_port(){
     INport=IOext_readReg(0x20,0x01) ;
     INport=(INport&0b00111000)/8;      //shift right 3 bit
     return(INport);
  }    */
  int Funkt::IOext_readReg(byte adr, byte reg){
      I2CMux.openChannel(6);
      Wire.beginTransmission(adr);
        Wire.write(reg);  // pointer
      if(Wire.endTransmission()!=0)Serial.println ("init error PCA9555"); 
       // delayMicroseconds (50);
      Wire.requestFrom(adr,2);
      while(Wire.available()){
          IOPort0= Wire.read(); 
          IOPort1= Wire.read(); 
        } 
    return ((IOPort1<8) +IOPort0);
  }

    void Funkt::IOext_writeReg(byte adr, byte reg, byte val_0,byte val_1){
      I2CMux.openChannel(6);
      Wire.beginTransmission(adr);  
      Wire.write(reg);  // pointer
      Wire.write(val_0);
      Wire.write(val_1);  
      Wire.endTransmission(); 
  }
   
