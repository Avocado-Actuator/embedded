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

#include "comms.h"
#include "motor.h"

#define CURRENT_CHANNELS 3
#define OFFSET_SAMPLES 20
#define CURRENT_SAMPLES 50
#define CSA_GAIN 20
#define MAX_CURRENT 2000

// store data read from ADC FIFO, must be as large as FIFO for sequencer in use
uint32_t isensereadings[4];
float isense[3];

void CurrentSenseInit(void);
void updateCurrent(void);

float getCurrent(void);
void setCurrent(float);
float getTargetCurrent(void);
void setTargetCurrent(float);
float getMaxCurrent(void);
void setMaxCurrent(float);

#endif /* ISENSE_H_ */
