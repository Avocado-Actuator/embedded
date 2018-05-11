/*
 * isense.h
 */

#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "utils/uartstdio.h"
#include "driverlib/pwm.h"
#include "driverlib/fpu.h"
#include "driverlib/timer.h"
#include "mag_encoder.h"
#include "reflectance.h"

//Velocity PID parameters
float KP_velocity;
float KI_velocity;
float KD_velocity;

float KP_angle;
float KI_angle;
float KD_angle;

float KP_current;
float KI_current;
float KD_current;

uint32_t Time;
uint8_t time_flag_200ms;
uint8_t time_flag_1000ms;
uint8_t time_flag_20ns;

void Timer0IntHandler(void);
void MotorInit(uint32_t);
void CurrentControl(float);
void VelocityControl(float);
void PositionControl(float);


float getAngle(void);
void setAngle(float);
float getTargetAngle(void);
void setTargetAngle(float);
void updateAngle(void);

float getVelocity(void);
void setVelocity(float);
float getTargetVelocity(void);
void setTargetVelocity(float);
void updateVelocity(void);

void PWMoutput(uint32_t, uint32_t);
uint32_t getPWM(void);

void brake(void);
void disableDriver(void);
void enableDriver(void);
#endif /* MOTOR_H_ */
