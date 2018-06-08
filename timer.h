#ifndef timer_H_
#define timer_H_

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/timer.h"

#include "motor.h"
#include "temp.h"

volatile uint32_t cur_ctl_flag;
volatile uint32_t vel_ctl_flag;
volatile uint32_t pos_ctl_flag;

void TimerInit(uint32_t);
void Timer0IntHandler(void);
void Timer1IntHandler(void);

uint32_t HEARTBEAT_TIME;

#endif /* timer_H_ */
