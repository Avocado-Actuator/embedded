/*
 * temp.h
 *
 *  Created on: Apr 30, 2018
 *      Author: Ryan
 */

#ifndef TEMP_H_
#define TEMP_H_

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"

void TempInit(uint32_t);
void GetTemp(void);

float CurrentTemp, PrevTemp;

#endif /* TEMP_H_ */
