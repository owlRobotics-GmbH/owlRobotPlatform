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
#include "owlcontrol.h"
#include "slcan.h"


class robot {
  public:
    robot(); 
    void begin();
    void roboter();
    void processReceivedPackets();
    bool weHaveControl; // do we have control over the motors?
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
    owlDrive leftMotor;     // owlDrive motor (CAN node)
    owlDrive rightMotor;    // owlDrive motor (CAN node)
    owlDrive sprayMotor;    // owlDrive motor (CAN node)
    owlControl control;     // owlControl PCB (CAN node)
    SLCAN slcan;  // CAN-USB-bridge
  private:
    bool plotDescr;
    unsigned long nextRequestTime; 
    unsigned long nextSerialOutputTime;
    unsigned long nextJoyTime;
    long unsigned timer_Relais, stateTimer1;
    float steerFac_l, steerFac_r;
    int controlLoops;
    unsigned long lastControlTime;

};



#endif
