#include "RunningMedian.h"
#include "config.h"
//#include "Funkt.h"

#define Joystk_y A1 //A2  28
#define Joystk_x A2 //A7  27
#define Joystk_z A3
 
#define Switch_Pin 20

#define invert_X 1
#define invert_Y 1
#define invert_Z 0


#define minJSval 0       // 430
#define maxJSval 1024    //4095
//#define ADC_resulution 10  //4096.0

#define ctr_Tol 60
class joystick {
  public:
   joystick(); 
   void begin();
   int get_Xaxis();
   int get_Yaxis();
   int get_Zaxis();
   bool buttonPressed();
   int resulution = pow(2,ADC_Resulution);
  private:
   RunningMedian<int,3> raw_x_history;
   RunningMedian<int,3> raw_y_history;
   RunningMedian<int,3> raw_z_history;
   int switchPin = Switch_Pin;
   int val_x,val_y,val_z;   // Joystick axis value (0-255)
   int raw_x,raw_y,raw_z;   // Raw data ADC
   int ctr_X, ctr_Y, ctr_Z; // Center pos. Joystick
   
   int i;                   //other var
};
