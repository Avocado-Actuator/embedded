/*
 * rs485.h
 *
 *  Created on: Apr 21, 2018
 *      Author: Ryan
 */

#ifndef RS485_H_
#define RS485_H_

#include <stdbool.h>
#include <stdint.h>
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

void RSInit(uint32_t);
void UARTIntHandler(void);
void UARTSend(const uint8_t*, uint32_t);

uint32_t uartSysClock;

#endif /* RS485_H_ */
