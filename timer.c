#include "timer.h"

/**
 * Initialize timers.
 *
 * @param g_ui32SysClock - system clock to sync with
 */
void TimerInit(uint32_t g_ui32SysClock) {
    // enable timer peripherals
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    // configure 32-bit periodic timers
    // 1ms timer
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock/1000);

    // 1s timer
    ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, g_ui32SysClock);

    // setup interrupts for the timer timeouts
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

    TIMER_0_TIME = 1;
    HEARTBEAT_TIME = 1;
}

/**
 * Handle timer interrupts, set a clock for program
 */
void Timer0IntHandler(void) {
    // Clear the timer interrupt.
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    TIMER_0_TIME++;
    if (TIMER_0_TIME % cur_int == 0) cur_ctl_flag = 1;
    if (TIMER_0_TIME % vel_int == 0) vel_ctl_flag = 1;
    if (TIMER_0_TIME % pos_int == 0) {
        pos_ctl_flag = 1;
        TIMER_0_TIME = 0;
    }

    // island time except for heartbeats
    ++HEARTBEAT_TIME;
    // expect heartbeat every 500 ms
    // user should send multiple in that time in case of corruption
    if(HEARTBEAT_TIME % 500 == 0) { brake(getEStop()); }
}

/**
 * Handle timer interrupts, set a clock for program
 */
void Timer1IntHandler(void){
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    updateTemp();
    // UARTprintf("Target Angle: %d\n", (int)TARGET_ANGLE);
    // UARTprintf("Angle: %d\n", (int)getAngle());
    // UARTprintf("Target Velocity: %d\n", (int)TARGET_VELO);
    // UARTprintf("Velocity: %d\n", (int)getVelocity());
    // UARTprintf("Target Current: %d\n", (int)TARGET_CUR);
    // UARTprintf("Current: %d\n", (int)getCurrent());
    // UARTprintf("outputPWN: %d\n\n", (int)duty);
    // UARTprintf("Temp: %d\n\n", (int)getTemp());
}
