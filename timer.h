#ifndef timer_H_
#define timer_H_

volatile uint32_t cur_ctl_flag;
volatile uint32_t vel_ctl_flag;
volatile uint32_t pos_ctl_flag;

void TimerInit(uint32_t);
void Timer0IntHandler(void);
void Timer1IntHandler(void);

uint32_t HEARTBEAT_TIME;

#endif /* timer_H_ */
