/*
 * comms.h
 *
 *  Created on: Apr 21, 2018
 *      Author: Ryan
 */

#ifndef comms_H_
#define comms_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "inc/hw_ints.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "crc.h"
#include "motor.h"
#include "isense.h"
#include "temp.h"
#include "timer.h"

void ConsoleInit(void);
void CommsInit(uint32_t);

void UARTIntHandler(void);
void UARTSend(const uint8_t*, uint32_t);
void UARTSetAddress(uint8_t);
uint8_t UARTGetAddress(void);

void UARTPrintFloat(float, bool);

void ConsoleIntHandler(void);
void UARTIntHandler(void);
void Timer0IntHandler(void);
void UARTSend(const uint8_t*, uint32_t);

uint8_t getAddress(void);
void setAddress(uint8_t);

uint32_t uartSysClock;

uint8_t recvIndex,
        ESTOP_HOLD,
        ESTOP_KILL,
        COMMAND_SUCCESS,
        COMMAND_FAILURE,
        OUTPUT_LIMITING,
        OUTPUT_FREE,
        STOP_BYTE,
        MAX_PAR_VAL,
        CMD_MASK,
        PAR_MASK,
        STATUS;

// getters/setters in motor
//status flags
uint8_t ESTOP_HOLD,
        ESTOP_KILL,
        COMMAND_SUCCESS,
        COMMAND_FAILURE,
        OUTPUT_LIMITING,
        OUTPUT_FREE;


// data structures
union Flyte {
  float f;
  uint8_t bytes[sizeof(float)];
};

enum Command {
    Get = 0,
    Set = 1
};

enum Parameter {
    Adr     = 0,
    Tmp     = 1,
    Cur     = 2,
    Vel     = 3,
    Pos     = 4,
    MaxCur  = 5,
    EStop   = 6,
    Status  = 7
};

#endif /* comms_H_ */
