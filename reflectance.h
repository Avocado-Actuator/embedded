/*
 * reflectance.h
 *
 *  Created on: Apr 1, 2018
 *      Author: allentang1996
 */

#ifndef REFLECTANCE_H_
#define REFLECTANCE_H_

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#define THRESH 3000
#define NUM_CHANNELS 4

//
// This array is used for storing the data read from the ADC FIFO. It
// must be as large as the FIFO for the sequencer in use.
uint32_t pui32ADC0Value[8];

void ReflectInit(void);
uint32_t getSection(void);
uint32_t bin2int(char* binArray, uint8_t size);
void gray2bin(char* gray, char* bin, uint8_t size);
float calcFinalAngle(uint32_t angle, uint32_t section);

#endif /* REFLECTANCE_H_ */
