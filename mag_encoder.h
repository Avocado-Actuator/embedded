#ifndef MAG_ENCODER_H_
#define MAG_ENCODER_H_

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"

#define NUM_SSI_DATA 3

// Number of cycles to average
#define AVG_CYCLES 50

// AS5048A Flags
#define CMD_READ    0x4000
#define CMD_WRITE   0x8000

// AS5048A Registers
#define REG_AGC         0x3ffd
#define REG_MAG         0x3ffe
#define REG_DATA        0x3fff
#define REG_CLRERR      0x1
#define REG_ZEROPOS_HI  0x0016
#define REG_ZEROPOS_LO  0x0017

uint32_t SSISysClock;

void EncoderInit(uint32_t);
static uint8_t calcEvenParity(uint32_t value);
uint32_t sendNOP(void);
void writeRegister(uint32_t addr, uint32_t data);
uint32_t readRegister(uint32_t addr);
void readData(uint32_t* angle, uint32_t* mag, char * agc, char * alarmHi, char * alarmLo);
void readAverageData(uint32_t* angle, uint32_t* mag, uint32_t* agc);
void zeroPosition(void);

#endif /* MAG_ENCODER_H_ */
