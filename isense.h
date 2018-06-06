/*
 * isense.h
 *
 *  Created on: Apr 20, 2018
 *      Author: Ryan
 */

#ifndef ISENSE_H_
#define ISENSE_H_

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "rs485.h"

#define CURRENT_CHANNELS 3
#define CURRENT_SAMPLES_NUM 20
#define CSA_GAIN 20
//
// This array is used for storing the data read from the ADC FIFO. It
// must be as large as the FIFO for the sequencer in use.
uint32_t isensereadings[4];
float isense[3];
float current_samples[CURRENT_SAMPLES_NUM];

void CurrentSenseInit(void);

float getCurrent(void);
void setCurrent(float);
float getTargetCurrent(void);
void setTargetCurrent(float);
void updateCurrent(void);

#endif /* ISENSE_H_ */
