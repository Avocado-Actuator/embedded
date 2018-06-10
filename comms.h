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

// <<<< INITS >>>>
void CommsInit(uint32_t);
void ConsoleInit(void);

// <<<< UTILITIES >>>>
uint8_t getAddress(void);
void setAddress(uint8_t);
void UARTPrintFloat(float, bool);

// <<<< MESSAGE HANDLING >>>>
void UARTSend(const uint8_t*, uint32_t, bool);

// <<<< HANDLERS >>>>
void UARTIntHandler(void);

uint32_t uartSysClock;

// status flags
// getters/setters in motor
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

// <<<< data structures >>>>

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

enum Mode {
    ModePos = 0,
    ModeVel = 1,
    ModeCur = 2
};
enum Mode MAIN_COMMAND_MODE;

#endif /* comms_H_ */
