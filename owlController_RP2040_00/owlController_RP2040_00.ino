/*
Only tested with legacy Arduino IDE v1.8.19 
Arduino boards to install:
+ Raspberry Pi Pico/RP2040 by Earle F. Philhower, III version 2.0.3
  https://github.com/earlephilhower/arduino-pico
  Open up the Arduino IDE and go to File->Preferences.
  In the dialog that pops up, enter the following URL in the "Additional Boards Manager URLs" field:
       https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json


Arduino libraries to install:
+ TCA9548A by Jonathan Dempsey Version 1.1.3
+ MCP342x by Steve Marple Version 1.0.4
+ Adafruit ILI9341 by Adafruit Version 1.5.12
+ Adafruit SSD1306 by Adafruit Version 2.5.7
+ Adafruit GFX by Adafruit Version 1.11.3 
+ Adafruit NeoPixel by Adafruit 1.10.6
+ NeoPixelConnect by Alan Yorinks 1.2.0
+ RP2040_PWM by Khoi Hoang Version 1.3.1


owlController (I/O PCB) wiring (PCB pins):

(NOTE: all pins including ADC pins max 3.3v input !)

ADC pin 3:
ADC pin 2:
ADC pin 1: charger voltage divider (charger detection - use a 75K resistor in series to reduce the voltage divider)
           (code assumes:  3v ADC input equals 30V sensor value)
IN pin 4:  BUMPER  (active-low)       -  PCB already contains a pull-up 
IN pin 3:  STOP BUTTON  (active-high) -  PCB already contains a pull-up
IN pin 2:  LIFT SENSOR (active-low)   -  PCB already contains a pull-up
IN pin 1:  SLOW-DOWN request (active-low) -  PCB already contains a pull-up

OUT pin 4:
OUT pin 3:
OUT pin 2:
OUT pin 1:


*/

//#include <Arduino.h>
//#include <stdio.h>
//#include <pico.h>
//#include "pico/stdlib.h"
//#include "hardware/structs/watchdog.h"
//#include <ld2410.h>
#include <Wire.h>
#include "Funkt.h"
#include "config.h"
#include "oledDisp.h"
#include "joystick.h"
#include "robot.h"
#include "comparser.h"
//#include "MPU9250.h"
#include <TCA9548A.h>
#include "mpu.h"
#include "mcp2515.h"
#include "NeoPix.h"
#include "owlcontrol.h"
#include "watchdog.h"
#include "can.h"

/*
extern "C" {
  #include <hardware/watchdog.h>
};
*/

#define blueLED 3
#define IntCAN 20
int odomTicksLeft,odomTicksMow,odomTicksRight;
int yxc;
long unsigned timer_Relais, startTimer, maxTime, endTimer, stateTimer[10];
long unsigned  MotorStatTimer;
//byte ii;
Funkt myF;
//MotorContr myMCtr;
oledDisp  oled;
joystick joystk;
robot robot;
NeoPix neopix ;
extern owlDrive owlDrive;
Watchdog watchdog;

ComParser comParser(&Serial);
//mpu mpu;
volatile unsigned long motorLeftTicksTimeout = 0;
volatile unsigned long motorRightTicksTimeout = 0;
volatile unsigned long motorMowTicksTimeout = 0;


volatile int odometry_left = 0;
volatile int odometry_ctrl = 0;
volatile int odometry_right = 0;  // temporary values for next processing

bool power_on_init = 1;

byte ii;

String picoID = "";


void setup() {
  delay (150);
  Serial.begin(115200);
  watchdog.begin();
  //Serial.print("start Setup SW Version : ");
  //Serial.println(VERS);
  pinMode(IntCAN,INPUT);  // Interrupt oin CAN
  /*if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
    } else {
        printf("Clean boot\n");
    }
  */

  can.begin();
  //Wire.setClock(400000);
  Wire.setClock(100000);  
  Wire.begin();
  delay (100);
  pinMode(blueLED,OUTPUT);
  myF.begin();
  oled.begin();
  joystk.begin();
  robot.begin();
  neopix.begin();
  myF.extPieper(1);
  delay (50);
  myF.extPieper(0);
  myF.anaMux(8);
  //Serial.println("Setup done");
  // default: 10 bit
  analogReadResolution(ADC_Resulution);
  //watchdog_enable(1500, 1);
  //mpu.begin();  //does not work jet


  // ---------------------------- get pico id --------------------------------    
  const int BOARD_ID_MESSAGE_LEN = 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1;
  char boardIDDesc[BOARD_ID_MESSAGE_LEN];
  pico_get_unique_board_id_string(boardIDDesc, BOARD_ID_MESSAGE_LEN);
  // E6616407E38B9125
  // E660C0D1C7268934
  // E4638C729F2E4821 
  pico_unique_board_id_t uid;
  pico_get_unique_board_id(&uid);
  unsigned short hash = 0;
  for (int i=0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++){
      hash = hash * 37 + uid.id[i];    
  }
  /*Serial.print("pico id:");  
  Serial.print(boardIDDesc);
  Serial.print(" ");
  Serial.print(hash);
  Serial.println();*/
  picoID = boardIDDesc;
  // ------------------------------------------------------------------    

  delay(3000);  
  //watchdog.dumpInfo();
  // start 2nd core 
  //Serial.print("starting 2nd core...");
  multicore_launch_core1(core1_entry); 
  watchdog.enable();
}

void loop() {

 if(millis()>PI_PWRon_delay&&power_on_init){        // PI_PWRon_delay is defined in config.h
    myF.LoadPowerPWM(10);
    if ( picoID == "E4638C729F2E4821"){
      //Serial.println("owlMower owlControlPCB");
      myF.PIpwr(0);
    } else {
      myF.PIpwr(1);     // set to 1 to power on the RP
    }
    if ( millis()>PI_PWRon_delay+1000){
      myF.CAN_Power(1);
      power_on_init=0;  // set init bit to 0, no more IF
    } 
  }

   startTimer=micros();

   // important tasks
   if (stateTimer[0]<millis()){

      
      if (robot.control.buzzerState) myF.extPieper(1);
        else myF.extPieper(0);

      stateTimer[0]=millis()+1; 
      //Serial.print(".");
   }

   if (stateTimer[1]<millis()){
      robot.roboter();
    //  myF.readIn_port();          //Port ext!! If(readIn_port .......     
           
      myF.OUT_Pin[1]=!myF.IN_Pin[1];
      myF.OUT_Pin[2]=!myF.IN_Pin[2];
      myF.OUT_Pin[3]=!myF.IN_Pin[3];
      myF.OUT_Pin[4]=!myF.IN_Pin[4];
      robot.control.setBumperState( (byte)(myF.IN_Pin[4] == LOW) ); 
      robot.control.setStopButtonState( (byte)(myF.IN_Pin[3] == HIGH) );
      if (!myF.IN_Pin[4])  neopix.NeoPixel_scene(1,1);      
      else if (!myF.IN_Pin[3]) neopix.NeoPixel_scene(2,1);
      else neopix.NeoPixel_scene(neopix.scene_default,neopix.default_brightness);
      myF.refreshIOports(); 
      stateTimer[1]=millis()+100;      
   } 

   if (stateTimer[5]<millis()){
      watchdog.resetTimeout();
      robot.control.batteryVoltage = myF.getBatteryVoltage();
      robot.control.chargerVoltage = myF.getChargerVoltage();
      robot.control.setRainState( myF.rain() );
      robot.control.setLiftState( (byte)(myF.IN_Pin[2] == LOW) );
      robot.control.setSlowDownState( (byte)(myF.IN_Pin[1] == LOW) );
      digitalWrite(blueLED,!digitalRead(blueLED)); 
      stateTimer[5]=millis()+1000;

      /*
      Serial.print("IN:");
      Serial.print(myF.IN_Pin[1]);
      Serial.print(",");      
      Serial.print(myF.IN_Pin[2]);
      Serial.print(",");      
      Serial.print(myF.IN_Pin[3]);
      Serial.print(",");      
      Serial.print(myF.IN_Pin[4]);
      Serial.print(" batV:");
      Serial.print(myF.getBatteryVoltage());
      Serial.print(" chgV:");
      Serial.print(myF.getChargerVoltage());
      Serial.print(" rain:");
      Serial.print(myF.rain());
      Serial.print(" mV:");
      Serial.print(myF.mVrain);    
      Serial.println();    
      */
   }
   
   // unimportant tasks
   if (stateTimer[8]<millis()){
      //Serial.println("ping");
      oled.status();
      //Serial.println("run done");
      stateTimer[8]=millis()+5000;
    
   } 

/*   if (stateTimer[9]<millis()){
     if(yxc++>2)  while(1){           // WD Test
        Serial.println ("WD Trap!! ");
      }
      stateTimer[9]=millis()+15000;
   }
 */  
   endTimer=micros();
   if(maxTime<=(endTimer-startTimer)) maxTime=endTimer-startTimer;
   //Serial.print ("max Time in uSek.: "); Serial.print (maxTime);
   //Serial.print ("Durchlauf in uSek.: "); Serial.println (endTimer-startTimer);
  //watchdog_update();


  /*
  //NeoPixel example
  
  if (millis()> MotorStatTimer){  
    MotorStatTimer = millis() + 750;
    //Serial.print("left :  ");
    robot.leftMotor.requestVelocity();
    //delay(7);  // no delays here to speed-up CAN packet receive processing
    //Serial.println(robot.leftMotor.velocity);
    //Serial.print("right :  ");
    robot.rightMotor.requestVelocity();
    //delay(7);   // no delays here to speed-up CAN packet receive processing
    //Serial.println(robot.rightMotor.velocity);
    
    if ((-1*robot.rightMotor.velocity)==0 && robot.leftMotor.velocity==0){   //Robot stops
      neopix.NeoPixel_scene(2,0.1);
      neopix.scene_default=2;
    }
    else if(abs(robot.rightMotor.velocity + robot.leftMotor.velocity)<=10){  // go ahead
      neopix.NeoPixel_scene(1,0.1);
      neopix.scene_default=1;
    }
    else if (robot.rightMotor.velocity >= -(1*robot.leftMotor.velocity*1.10)){  //Robot left turn
      neopix.NeoPixel_scene(6,0.1);
      neopix.scene_default=6;
    }
    else if ((-1*robot.leftMotor.velocity)>= robot.rightMotor.velocity*1.10){  //Robot right turn
      neopix.NeoPixel_scene(7,0.1);
      neopix.scene_default=7;
    }    
    else{
      neopix.NeoPixel_scene(0,0.1);
      neopix.scene_default=0;
    }
  }
  */


}


// 2nd core loop 
// https://github.com/arduino/ArduinoCore-mbed/issues/217
// https://forums.raspberrypi.com/viewtopic.php?t=302472
// https://hackaday.io/page/9880-raspberry-pi-pico-multicore-adventures

void core1_entry(){
  unsigned long nextMiscTime = 0;
  while (true){
    
    // ---- handling for Linux CAN-USB bridge (SLCAN) -------------
    robot.slcan.fillRxFifo(); // USB packet receiver (FIFO)
    can.fillRxFifo(); // CAN packet receiver (FIFO)
    can.processTxFifo();  // CAN packet sender (FIFO)    

    robot.slcan.run();     // process CAN-USB-bridge 
    robot.processReceivedPackets();  // process received CAN packets
    // ----------------------------------------------------------

    /*if (millis() > nextMiscTime){
      nextMiscTime = millis() + 500;
      //watchdog_update();
      watchdog.resetTimeout();                               
    }*/
    //delay(1);
  }
}

  

