/*
Arduino boards to install:
+ Raspberry Pi Pico/RP2040 by Earle F. Philhower, III version 2.0.3

Arduino libraries to install:
+ TCA9548A by Jonathan Dempsey Version 1.1.3
+ MCP342x by Steve Marple Version 1.0.4
+ Adafruit ILI9341 by Adafruit Version 1.5.12
+ Adafruit SSD1306 by Adafruit Version 2.5.7
+ Adafruit GFX by Adafruit Version 1.11.3 
+ RP2040_PWM by Khoi Hoang Version 1.3.1

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

extern "C" {
  #include <hardware/watchdog.h>
};

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

void setup() {
  delay (50);
  pinMode(IntCAN,INPUT);  // Interrupt oin CAN
  Serial.begin(115200);
  Serial.println("start Setup");

  if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
    } else {
        printf("Clean boot\n");
    }

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
  Serial.println("Setup done");
  // default: 10 bit
  analogReadResolution(ADC_Resulution);
  watchdog_enable(1500, 1);
  //mpu.begin();  //does not work jet
}

void loop() {

 if(millis()>1500&&power_on_init){
    myF.PIpwr(1);     // set to 1 to power on the RP
    power_on_init=0;  // set init bit to 0, no more IF
  }

   startTimer=micros();

   // important tasks
   if (stateTimer[0]<millis()){
       
      stateTimer[0]=millis()+50; 
   }
    
   if (stateTimer[1]<millis()){
      robot.roboter();
    //  myF.readIn_port();          //Port ext!! If(readIn_port .......     
      

      myF.OUT_Pin[1]=!myF.IN_Pin[1];
      myF.OUT_Pin[2]=!myF.IN_Pin[2];
      myF.OUT_Pin[3]=!myF.IN_Pin[3];
      myF.OUT_Pin[4]=!myF.IN_Pin[4];
      if (!myF.IN_Pin[4])  neopix.NeoPixel_scene(1,1);
      else if (!myF.IN_Pin[3]) neopix.NeoPixel_scene(2,1);
      else neopix.NeoPixel_scene(neopix.scene_default,neopix.default_brightness);
      myF.refreshIOports(); 
      stateTimer[1]=millis()+100;
   } 

   if (stateTimer[5]<millis()){
      myF.rain();                 // If(rain .......
      digitalWrite(blueLED,!digitalRead(blueLED)); 
      stateTimer[5]=millis()+1000;
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
  watchdog_update();

//NeoPixel example
  if (millis()> MotorStatTimer){  
    MotorStatTimer = millis() + 750;
    Serial.print("left :  ");
    robot.leftMotor.requestVelocity();
    delay(7);
    Serial.println(robot.leftMotor.velocity);
    Serial.print("right :  ");
    robot.rightMotor.requestVelocity();
    delay(7);
    Serial.println(robot.rightMotor.velocity);
    
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
}



  
