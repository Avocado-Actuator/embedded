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

uint32_t Time;
uint8_t time_flag_200ms;
uint8_t time_flag_1000ms;
uint8_t time_flag_2ms;

uint32_t TargetVelocity;
uint32_t CurrentVelocity;
uint32_t currentVeloError;
uint32_t lastVeloError;
uint32_t prevVeloError;
uint32_t frequency; //2500-9000
uint32_t duty;

uint32_t PrevAngle;
uint32_t CurrentAngle;
uint32_t TargetAngle;
uint32_t currentAngleError;
uint32_t lastAngleError;
uint32_t prevAngleError;

void Timer0IntHandler(void);
void MotorInit(uint32_t);
void VelocityControl(void);
void GetAngle(void);
void GetVelocity(void);

#endif /* MOTOR_H_ */
