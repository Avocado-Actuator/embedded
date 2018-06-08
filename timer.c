#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/timer.h"

#include "timer.h"
#include "motor.h"

void TimerInit(uint32_t g_ui32SysClock)
{
    /************** Initialization for timer (1ms and 1s)  *****************/
    //Enable the timer peripherals
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    // Configure 32-bit periodic timers.
    //1ms timer
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock/1000);//trigger every 1ms
    //1s timer
    ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, g_ui32SysClock);//trigger every 1s
    // Setup the interrupts for the timer timeouts.
    ROM_IntEnable(INT_TIMER0A);
    ROM_IntEnable(INT_TIMER1A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    // Enable the timers.
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);
    ROM_TimerEnable(TIMER1_BASE, TIMER_A);

    cur_ctl_flag = 0;
    vel_ctl_flag = 0;
    pos_ctl_flag = 0;
}

int timer0_cnt = 0;
int count=0;

//*****************************************************************************
// Handler of timer interrupts, set a clock for program
//
// input: None
//
// return: None
//*****************************************************************************
void Timer0IntHandler(void) {
    // Clear the timer interrupt.

    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    timer0_cnt++;
    if ((timer0_cnt % cur_int) == 0){
        cur_ctl_flag = 1;
    }
    if (timer0_cnt%vel_int == 0){
        vel_ctl_flag = 1;
    }
    if (timer0_cnt%pos_int == 0){
        pos_ctl_flag = 1;
        timer0_cnt = 0;
    }
//    updateAngle();
//    updateVelocity();
//    updateCurrent();

//    PWMoutput(TARGET*10);

    //PWMoutput(-20);
//    PositionControl(TARGET_ANGLE);
    //VelocityControl(TARGET_VELO);

}

void Timer1IntHandler(void){
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
//    updateAngle();
//    UARTprintf("Angle:%d\n\n",(int)getAngle());

    count=0;
    UARTprintf("Target Angle: %d\n", (int)TARGET_ANGLE);
    UARTprintf("Angle: %d\n", (int)getAngle());
    UARTprintf("Target Velocity: %d\n", (int)TARGET_VELO);
    UARTprintf("Velocity: %d\n", (int)getVelocity());
    UARTprintf("Target Current: %d\n", (int)TARGET_CUR);
    UARTprintf("Current: %d\n", (int)getCurrent());
    UARTprintf("outputPWN: %d\n\n", (int)duty);
//    UARTprintf("duty: %d\n\n", (int)duty);


//    UARTprintf("Target:%d\n",TARGET);
//    UARTprintf("Angle:%d\n\n",(int)getAngle());
//    UARTprintf("outVelo:%d\n",(int)outputVelo);
//    UARTprintf("Velo:%d\n\n",(int)getVelocity());
//    UARTprintf("OutputPWM:%d\n\n",(int)outputCurrent);

    //updateTemp();
    //UARTprintf("Temp: %d\n", (int)getTemp());
}
