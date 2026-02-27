#include "comparser.h"
#include "robot.h"
#include "joystick.h"
#include "cmd.h"
#include "Funkt.h"
#include "config.h"


extern robot robot;
extern ComParser comParser;
extern Funkt myF;
extern joystick joystk;


//#define DEBUG 1

cmd::cmd(){}

/*-----------   Cmd. AT+M     ----------------- */
void cmd::cmdM(){

  String s;
  if (comParser.nextStringToken(s)){     //s 
  //Serial.println(s);
      if (comParser.nextIntToken(atLeftSpeed)){
        if (comParser.nextIntToken(atRightSpeed)){ 
          if (comParser.nextIntToken(atSpraySpeed)){
            if (comParser.nextIntToken(atPressureEnable)){             
              if (comParser.nextIntToken(atAllowManualCtl)){ 
                #ifdef DEBUG
                  Serial.print("motor request: ");
                  Serial.print(atLeftSpeed);
                  Serial.print(",");
                  Serial.print(atRightSpeed);
                  Serial.print(",");
                  Serial.print(atSpraySpeed);    
                  Serial.print(",");               
                  Serial.print(atPressureEnable);    
                  Serial.print(",");                                                              
                  Serial.println(atAllowManualCtl);                                
                #endif  
                robot.allowManualCtl = (atAllowManualCtl != 0); 
                 robot.allowManualCtl=0; // overwride              
                if (!robot.allowManualCtl){
                  robot.speed_left = atLeftSpeed;
                  robot.speed_right = atRightSpeed;
                }
                robot.speed_spray = atSpraySpeed;
                robot.pressure_enable = atPressureEnable;
                ans = "M,";
                ans += robot.odometry_left_sum;
                ans += ",";          
                ans += robot.odometry_right_sum;
                ans += ",";
                ans += robot.odometry_ctrl_sum; 
                ans += ",";  
                ans += robot.batteryVolt;
                ans += ",";
                ans += robot.X_axis;
                ans += ",";
                ans += robot.Y_axis;
                ans += ",";
                ans += robot.Z_axis;
                ans += ",";
                ans += joystk.buttonPressed();
                ans += ",";              
                ans += robot.emergStop;
                ans += ",";                            
                ans += robot.bumperStop;
                ans += ",";
                ans += robot.leftMotor.error;
                ans += ",";
                ans += robot.rightMotor.error;
                ans += ",";
                ans += robot.sprayMotor.error;              
                // ... add more interesting data for the master (like current odometry counters etc.)
                comParser.sendAnswer(ans);  // send realtime sensor data
                robot.watchdogTimer=millis()+TimeWD;
              }
            }
          }
        }
      }
    }
  }

/*-----------   Cmd. AT+S     ----------------- */
      
void cmd::cmdS(){  
              ans = "S,";
              ans += robot.X_axis;
              ans += ",";
              ans += robot.Y_axis;
              ans += ",";
              ans += robot.Z_axis;
              ans += ",";
              ans += robot.odometry_ctrl_sum; 
              ans += ",";              
              ans += robot.odometry_left_sum;
              ans += ",";          
              ans += robot.odometry_right_sum;
              ans += ",";
          /*    ans += myF.read_M_Current(2);     //Motor current centr                  
              ans += ",";
              ans += myF.read_M_Current(1);     //Motor current left
              ans += ",";
              ans += myF.read_M_Current(3);     //Motor current right
              ans += ",";*/
              ans += myF.rain();                // Rain detected bool  
              ans += ",";
              ans += robot.batteryVolt; // Battery power
              ans += ",";              
              ans += joystk.buttonPressed();
        comParser.sendAnswer(ans);   // send version info
      }   

/*-----------   Cmd. AT+J     ----------------- */
void cmd::cmdJ(){  // master button/joystick info
              ans = "J,";
              ans += robot.X_axis;
              ans += ",";
              ans += robot.Y_axis;
              ans += ",";
              ans += robot.Z_axis;
              ans += ",";       
              ans += joystk.buttonPressed();              
         comParser.sendAnswer(ans);   // send version info
      }

/*-----------   Cmd. AT+V     ----------------- */
void cmd::cmdV(){  // master requests version info
        String ans = "V,";
        ans += VERS; 
        comParser.sendAnswer(ans);   // send version info
      }
void cmd::cmdB(){  // Buzzer toggel
        String ans = "B,";
        ans += !digitalRead(PIEPER_Pin); 
        comParser.sendAnswer(ans);   // send version info
        digitalWrite(PIEPER_Pin,!digitalRead(PIEPER_Pin));
      }  
void cmd::cmdP(){  // Power off uController Board
        String ans = "P,";
        ans += 0; 
        comParser.sendAnswer(ans);   // send version info
        digitalWrite(PIEPER_Pin,1);
        delay(75);
        digitalWrite(PIEPER_Pin,0);
        myF.SW_Power_off();
      }       
      
/*-----------   no Cmd. found      ----------------- */
void cmd::NOcmd(){
        String ans = "?,";
        ans += "use M,mow,left,right for motor speed; V for versiom; S for summery; J for joystick axis" ; 
        comParser.sendAnswer(ans);   // send version info  
}
