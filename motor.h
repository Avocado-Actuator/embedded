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

int flag;

uint32_t Time;
uint8_t time_flag_200ms;
uint8_t time_flag_1000ms;
uint8_t time_flag_10000ms;
uint8_t time_flag_2ms;

void Timer0IntHandler(void);
void MotorInit(uint32_t);
void VelocityControl(void);
void PositionControl(void);
void PositionControlCS(void);

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

uint8_t* getEStop(void);
void setEStop(uint8_t);
uint8_t* getStatus(void);
void setStatus(uint8_t);
//status flags
uint8_t ESTOP_HOLD,
        ESTOP_KILL,
        COMMAND_SUCCESS,
        COMMAND_FAILURE,
        OUTPUT_LIMITING,
        OUTPUT_FREE;

void testSpin(uint32_t, uint32_t);
void brake(void);
void disableDriver(void);
#endif /* MOTOR_H_ */
