#include <Arduino.h>
//#include "MotorContr.h"
#include "Funkt.h"
#include "joystick.h"
#include "robot.h"
#include "comparser.h"
#include "config.h"
#include "cmd.h"
#include "mcp2515.h"

//#define DEBUG 1

#define MOTOR_LEFT_NODE_ID  1
#define MOTOR_RIGHT_NODE_ID 2
#define MOTOR_SPRAY_NODE_ID 3


extern Funkt myF;
cmd cmdAT;
//extern MotorContr myMCtr;

extern joystick joystk;
extern ComParser comParser;
bool debugOutput = false;
  
float yExp = 0;
float xExp = 0;



// this CAN driver connects CAN packet interface with owlDrives
class MyCanDriver: public owlDriveCAN {
  public:       
    MCP2515 can0;
    MyCanDriver() : can0(spi0, 17, 19, 16, 18, 10000000) {
      //Initialize interface
      can0.reset();
      can0.setBitrate(CAN_1000KBPS, MCP_16MHZ);
      //can0.setBitrate(CAN_1000KBPS, MCP_8MHZ);    
      can0.setNormalMode();
    } 

    // send packet via CAN interface
    void sendPacket(unsigned long id, int len, unsigned char data[8]) override {
      struct can_frame buf;
      buf.can_id = id;    
      buf.can_dlc = len;
      for (int i=0; i < 8; i++) buf.data[i] = data[i];
      //Serial.print("sendPacket ");
      //Serial.println(id);
      can0.sendMessage(&buf);
    };

    // if we received a CAN packet via CAN interface, send it to all owlDrives
    void processReceivedPackets(robot *aRobot){
      struct can_frame rx;
      while (can0.readMessage(&rx) == MCP2515::ERROR_OK) {
        //Serial.print("New frame from ID: ");
        //Serial.println(rx.can_id);
        aRobot->leftMotor.onCanReceived(rx.can_id, rx.can_dlc, rx.data);
        aRobot->rightMotor.onCanReceived(rx.can_id, rx.can_dlc, rx.data);
        aRobot->sprayMotor.onCanReceived(rx.can_id, rx.can_dlc, rx.data);            
      }
    }
};


MyCanDriver canDriver;



robot::robot() : 
    leftMotor(&canDriver, MOTOR_LEFT_NODE_ID),  
    rightMotor(&canDriver, MOTOR_RIGHT_NODE_ID),  
    sprayMotor(&canDriver, MOTOR_SPRAY_NODE_ID) {    
  
  lastControlTime = 0;
  nextJoyTime = 0;
  nextJoyTime = 0;

  speed_left = 0; // set-speed (rpm) 
  speed_right = 0;
  speed_spray = 0;

  linearSpeedSet = 0;
  angularSpeedSet = 0;
  
  allowManualCtl = true;
  nextSerialOutputTime = 0;
  nextRequestTime = 0;
}

void robot::begin(){

 }

void robot::roboter(){
  
  batteryVolt = myF.ADC_read(7)/1000*30.0+0.65; // read Battery power

  bool joystickButtonPressed = joystk.buttonPressed();
  bool emergencyPressed = (emergStop != 0);
  bool bumperPressed = (bumperStop != 0);
  bool watchDogTimeout = (millis()>watchdogTimer);
  if (watchDogTimeout){
    allowManualCtl = true;
  }
  #ifdef DEBUG
    debugOutput = true;    
  #endif
  
  //***************************     Joystick reading  ***************************************************    
  // normalize: (-100..+100)
  X_axis = int( ((float)joystk.get_Xaxis())/((float)joystk.resulution)*256.0 ); //256
  Y_axis = int( ((float)joystk.get_Yaxis())/((float)joystk.resulution)*256.0 );
  Z_axis = int( ((float)joystk.get_Zaxis())/((float)joystk.resulution)*256.0 );

  /*
  if (abs(X_axis) > 80){
    Serial.println(X_axis);
  }
  if (abs(Y_axis) > 80){
    Serial.println(Y_axis);
  }
  */

  String cmd;

  //***************************     AT command handling  ****************************      
  while (comParser.cmdAvailable(cmd)){
    #ifdef DEBUG
      Serial.print("CMD: ");
      Serial.println(cmd);
    #endif
    if (cmd.startsWith("AT+D")) debugOutput = !debugOutput;       
    else if (cmd.startsWith("AT+V")) cmdAT.cmdV(); 
    else if (cmd.startsWith("AT+M"))cmdAT.cmdM();
    else if (cmd.startsWith("AT+S"))cmdAT.cmdS();
    else if (cmd.startsWith("AT+J"))cmdAT.cmdJ();
    else if (cmd.startsWith("AT+B"))cmdAT.cmdB();
    else if (cmd.startsWith("AT+P"))cmdAT.cmdP();
    else cmdAT.NOcmd();
  }

  //if ((allowManualCtl) && (joystickButtonPressed)){
  allowManualCtl=0;         // overright
  if (allowManualCtl){
    //***************************     Joystick to motor speed, direction etc  ***************************************************
    if (millis() > nextJoyTime){
      #define MAX_X 150.0  // joystick max X
      #define MAX_Y 150.0  // joystick max Y
      #define MAX_SPEED 1500  // motorwelle rad/s  (80 / 150) 

      float joyX = min(MAX_X, max(X_axis, -MAX_X));
      float joyY = min(MAX_Y, max(Y_axis, -MAX_Y));

      float threshX = MAX_X * 0.7; 
      float threshY = MAX_Y * 0.7;

      bool hasAngularSpeed = (abs(angularSpeedSet) > 0.01);
      
      if (joyX < -threshX){  // turn right
        //linearSpeedSet = 0;
        //if (angularSpeedSet < 0) angularSpeedSet = 0;        
        angularSpeedSet = min(MAX_SPEED, angularSpeedSet + 50);
        nextJoyTime = millis() + 50;      //150
      }
      else if (joyX > threshX){  // turn left    
        //linearSpeedSet = 0;
        //if (angularSpeedSet > 0) angularSpeedSet = 0;
        angularSpeedSet = max(-MAX_SPEED, angularSpeedSet - 50);
        nextJoyTime = millis() + 50;      
      }
      else if (joyY > threshY){ // forward        
        angularSpeedSet = 0;
        //if (linearSpeedSet < 0) linearSpeedSet = 0;        
        if (!hasAngularSpeed) linearSpeedSet = min(MAX_SPEED, linearSpeedSet + 50);       
        nextJoyTime = millis() + 50;      
      }
      else if (joyY < -threshY){  // backwards
        angularSpeedSet = 0;
        //if (linearSpeedSet > 0) linearSpeedSet =0;
        if (!hasAngularSpeed) linearSpeedSet = max(-MAX_SPEED, linearSpeedSet - 50);       
        nextJoyTime = millis() + 50;    
      }

      float angularSpeed = angularSpeedSet;
      if (linearSpeedSet < 0) angularSpeed *= -1;

      speed_left = linearSpeedSet + angularSpeed /2;          
      speed_right = linearSpeedSet - angularSpeed / 2;          
    
      /*
      xExp = exp(abs(joyX) / MAX_X * 6.0 - 3.0) / 20.0 * MAX_SPEED;
      if (joyX < 0) xExp *= -1;
      yExp = exp(abs(joyY) / MAX_Y * 6.0 - 3.0) / 20.0 * MAX_SPEED;
      if (joyY < 0) yExp *= -1;
      
      speed_left = yExp - xExp;
      speed_right = yExp + xExp;*/        
    }
  } else {
    linearSpeedSet = 0;
    angularSpeedSet = 0;
  }
  
  //***************************   security check   ***************************************************
  
  //if ( ((!joystickButtonPressed) &&  (watchDogTimeout)) || (emergencyPressed) || (bumperPressed) ) {
//  bumperPressed=0;         //overwrite button 
//  emergencyPressed=0;      //overwrite button 
//  joystickButtonPressed=0;  //overwrite button 
  if ( (joystickButtonPressed) || (emergencyPressed) || (bumperPressed) ) {  
    #ifdef DEBUG
      if (emergencyPressed) Serial.println("emergency");
      if (bumperPressed) Serial.println("bumper");
      //if (watchDogTimeout) Serial.println("watchDogTimeout");
    #endif
    linearSpeedSet = 0;
    angularSpeedSet = 0;
    speed_left = 0;
    speed_right = 0;
    speed_spray = 0;
  }

  // send speed to motor
  //leftMotor.sendVelocity(speed_left);
  //rightMotor.sendVelocity(speed_right);  
  //sprayMotor.sendVelocity(speed_spray);

  if (millis()> nextRequestTime){  
    nextRequestTime = millis() + 200;
    //leftMotor.requestError();
    //leftMotor.requestVelocity();
    
    //rightMotor.requestError();
    //rightMotor.requestVelocity();
  
    //sprayMotor.requestError();
    //sprayMotor.requestVelocity();
  }
  
  canDriver.processReceivedPackets(this);

  
  if (millis() > nextSerialOutputTime){
    if (debugOutput){
      nextSerialOutputTime = millis() + 1000;
      Serial.print("ctFrq=");
      Serial.print(controlLoops);
    
      Serial.print (" joystick(x,y,z)="); 
      Serial.print (X_axis);
      Serial.print (",");      
      Serial.print (Y_axis);
      Serial.print (",");      
      Serial.print (Z_axis);
    
      Serial.print(" exp(x,y)=");
      Serial.print(xExp);
      Serial.print(",");
      Serial.print(yExp);

      Serial.print(" spd_lin_ang=");
      Serial.print(linearSpeedSet); 
      Serial.print(",");    
      Serial.print(angularSpeedSet); 

      Serial.print(" spd_left_right=");
      Serial.print(speed_left); 
      Serial.print(",");    
      Serial.print(speed_right); 

      Serial.print(" err=");
      Serial.print(leftMotor.error); 
      Serial.print(",");    
      Serial.print(rightMotor.error);
      Serial.print(",");    
      Serial.print(sprayMotor.error);       
      
      Serial.print(" vel=");
      Serial.print(leftMotor.velocity); 
      Serial.print(",");    
      Serial.print(rightMotor.velocity);
      Serial.print(",");    
      Serial.print(sprayMotor.velocity);       
      
      Serial.println();


      controlLoops = 0;    
    }
  }

  // ---------------------------------------------------------------------------------------
  controlLoops++;


  //     Serial.print ("  Switch "); Serial.print (digitalRead(joystk.Switch));       
  //    Serial.print (" speed left "); Serial.print (speed_left);
  //     Serial.print ("  speed right "); Serial.println (speed_right);



}
