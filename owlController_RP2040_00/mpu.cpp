/*
#include <Wire.h>
#include <TCA9548A.h>
#include <MPU6050.h>
#include "mpu.h"

extern TCA9548A I2CMux;


MPU6050 mpu6050;

mpu::mpu(){}
void mpu::begin() {
  I2CMux.openChannel(4);
  delay(1);
  Serial.println("Initialize MPU6050");
  mpu6050.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G);
  Vector rawAccel = mpu6050.readRawAccel();
  Vector normAccel = mpu6050.readNormalizeAccel();

  Serial.print(" Xraw = ");
  Serial.print(rawAccel.XAxis);
  Serial.print(" Yraw = ");
  Serial.print(rawAccel.YAxis);
  Serial.print(" Zraw = ");

  Serial.println(rawAccel.ZAxis);
  Serial.print(" Xnorm = ");
  Serial.print(normAccel.XAxis);
  Serial.print(" Ynorm = ");
  Serial.print(normAccel.YAxis);
  Serial.print(" Znorm = ");
  Serial.println(normAccel.ZAxis);

}
*/
