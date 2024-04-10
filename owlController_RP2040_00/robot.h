#ifndef ROBOT_H
#define ROBOT_H

#define softAccelerate  0.5   //0.1
#define slowDown  0.5     //0.5
#define allowZeroTurn 1
#define maxZTspeed 100
#define onZTfact 0.2

#include "config.h"
#include "pid.h"
#include "can.h"
#include "owldrive.h"



class robot {
  public:
    robot(); 
    void begin();
    void roboter();
    bool allowManualCtl;
    bool emergStop, bumperStop;
    int X_axis, Y_axis, Z_axis, X_alt, Y_alt, Z_alt; 
    float linearSpeedSet; // set-speed (rad/s)
    float angularSpeedSet; // set-speed (rad/s)    
    int speed_left, speed_right;   // set-speed (rad/s) 
    int speed_spray;  // Düsenmotorgeschwindigkeit
    bool pressure_enable;  // Hochdruckreiniger einschalten?
    float batteryVolt;
    long unsigned watchdogTimer;
    unsigned long odometry_ctrl_sum;
    unsigned long odometry_left_sum;
    unsigned long odometry_right_sum;
    owlDrive leftMotor;    
    owlDrive rightMotor;
    owlDrive sprayMotor;
  private:
    bool plotDescr;
    unsigned long nextRequestTime; 
    unsigned long nextSerialOutputTime;
    unsigned long nextJoyTime;
    long unsigned timer_Relais, stateTimer1;
    float steerFac_l, steerFac_r;
    int controlLoops;
    unsigned long lastControlTime;
    void sendCanData(int destNodeId, canCmdType_t cmd, canValueType_t val, canDataType_t data);
    void onCanReceive(struct can_frame buf);
    void printCanFrame(struct can_frame buf);        

};

#endif
