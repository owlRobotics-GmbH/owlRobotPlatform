
#include <Arduino.h>
//#include "MotorContr.h"
#include "joystick.h"
#include "Funkt.h"

//extern MotorContr myMCtr;
extern Funkt myF;

joystick::joystick(){}

bool joystick::buttonPressed(){
   //return (digitalRead(switchPin) == LOW);
   return (myF.IN_Pin[1]==LOW);
}
   

void joystick::begin(){                   //find joystick center  position 
  pinMode ( switchPin, INPUT_PULLUP ) ;
  delay(10);
  ctr_X = analogRead(Joystk_x);
  ctr_Y = analogRead(Joystk_y);
  ctr_Z = analogRead(Joystk_z);
  for (i=0;i<=9;i++){
      delay(2);
      ctr_X = ctr_X + analogRead(Joystk_x);
      delay(2);
      ctr_Y = ctr_Y + analogRead(Joystk_y);
      delay(2);
      ctr_Z = ctr_Z + analogRead(Joystk_z);
  }
  ctr_X = int(ctr_X/++i);
  ctr_Y = int(ctr_Y/i);
  ctr_Z = int(ctr_Z/i);
}

int joystick::get_Xaxis(){
  raw_x_history.add(analogRead(Joystk_x));
  raw_x_history.add(analogRead(Joystk_x));  
  raw_x_history.getMedian(raw_x);    
  if( raw_x <= ctr_X-ctr_Tol){
    val_x = ctr_X - ctr_Tol - raw_x;
    val_x= int( -1.0*val_x /(ctr_X-ctr_Tol-minJSval)*resulution);
  } else if(( raw_x >= ctr_X+ctr_Tol)){
    val_x=(raw_x-ctr_X+ctr_Tol);
    val_x= int(1.0* val_x / (maxJSval-ctr_X+ctr_Tol)*resulution);
  }else val_x=0;
  if( val_x>resulution) val_x = resulution;
  if( val_x<(resulution* -1)) val_x = resulution* -1;
  if (invert_X&&val_x!=0) val_x = val_x * -1;
  return (int(val_x)) ;
}

int joystick::get_Yaxis(){
  raw_y_history.add(analogRead(Joystk_y));  
  raw_y_history.add(analogRead(Joystk_y));
  raw_y_history.getMedian(raw_y);    
  if( raw_y <= ctr_Y-ctr_Tol){
    val_y= (ctr_Y-ctr_Tol - raw_y);
    val_y= int(-1.0* val_y / (ctr_Y-ctr_Tol-minJSval) *resulution);
  } else if(( raw_y >= ctr_Y+ctr_Tol)){
    val_y=(raw_y-ctr_Y+ctr_Tol);
    val_y= int(1.0* val_y / (maxJSval-ctr_Y+ctr_Tol)*resulution);
  }else val_y=0;
  if( val_y>resulution) val_y = resulution;
  if( val_y<(resulution* -1)) val_y = resulution* -1; 
  if (invert_Y&&val_y!=0) val_y = val_y * -1; 
  return (val_y) ;//val_y
}

int joystick::get_Zaxis(){
  raw_z_history.add(analogRead(Joystk_z));  
  raw_z_history.add(analogRead(Joystk_z));
  raw_z_history.getMedian(raw_z);    
  if( raw_z <= ctr_Z-ctr_Tol){
    val_z= (ctr_Z-ctr_Tol - raw_z);
    val_z= int(-1.0* val_z / (ctr_Z-ctr_Tol-minJSval) *resulution);
  } else if(( raw_z >= ctr_Z+ctr_Tol)){
    val_z=(raw_z-ctr_Z+ctr_Tol);
    val_z= int(1.0* val_z / (maxJSval-ctr_Z+ctr_Tol)*resulution);
  }else val_z=0;
  if (invert_Y&&val_y!=0) val_y = val_y * -1;
  if( val_z>resulution) val_z = resulution;
  if( val_z<(resulution* -1)) val_z = resulution* -1;  
  if (invert_Z&&val_z!=0) val_z = val_z * -1;
  return (val_z) ;
}

/*
 * Mathematisch ist die Sache sehr einfach:

  u = umax * e ** ln(s)

s     ist die Stellgröße (Potiposition)
umax  ist die ans Poti angelegte Spannung
ln(s) ist die Spannung, die aus dem log. Poti bei Stellung s rauskommt
e     ist die eulersche Zahl e = 2,718281828459...
**    ist der Potenzieroperator
u     ist die linearisierte Größe
*/
 

 
