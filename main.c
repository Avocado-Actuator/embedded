/*
 * Main file for the Avocado embedded code
 */

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"
// WE SHOULDNT NEED THIS
// BUT WE DO, DONT TOUCH IT
#include "utils/uartstdio.c"

#include "isense.h"
#include "comms.h"
#include "reflectance.h"
#include "mag_encoder.h"
#include "motor.h"
#include "temp.h"

// System clock rate in Hz.
volatile uint32_t g_ui32SysClock;

/**
 * Called if driver library encounters an error
 */
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) { }
#endif

//*****************************************************************************
// Handler of timer interrupts, set a clock for program
//
// input: None
//
// return: None
//*****************************************************************************
int timer0_cnt = 0;
int count=0;
volatile uint32_t cur_ctl_flag = 0;
volatile uint32_t vel_ctl_flag = 0;
volatile uint32_t pos_ctl_flag = 0;

void Timer0IntHandler(void)
{
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


//*****************************************************************************
//
// Main function
//
//*****************************************************************************

//*****************************************************************************
// SUPER IMPORTANT, CCS IS JANK, NEED TO INCREASE STACK SIZE MANUALLY IN ORDER
// TO USE SPRINTF, GO TO Build -> ARM Linker -> Basic Options IN PROJECT
// PROPERTIES SET C SYSTEM STACK SIZE TO 65536
//*****************************************************************************
int
main(void) {
    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                                 SYSCTL_OSC_MAIN |
                                                 SYSCTL_USE_PLL |
                                                 SYSCTL_CFG_VCO_480), 120000000);

    ROM_IntMasterEnable(); // Enable processor interrupts.

    ConsoleInit(); // Initialized UART0 for console output using UARTStdio
    UARTprintf("\n\nTiva has turned on...\n");
    //UARTSend((uint8_t *)"\033[2JTiva has turned on\n\r", 24);

    UARTprintf("Initializing...\n");
    //RSInit(g_ui32SysClock); // Initialize the comms link

    EncoderInit(g_ui32SysClock);
    ReflectInit();
    TempInit(g_ui32SysClock);
    MotorInit(g_ui32SysClock);
    CurrentSenseInit();
    TimerInit(g_ui32SysClock);

    UARTprintf("Initialized\n\n");


    PWMoutput(0);
    //disableDriver();


    while(1)
    {
        // Check the busy flag in the uart7 register. If not busy, set transceiver pin low
//        if (UARTReady()){
//            UARTSetRead();
//        }
//        UARTprintf("Start\n");
//        int i;
//        for (i =0; i < 100000; i++) {
//            updateCurrent();
//        }
//        UARTprintf("Stop\n");
//        UARTprintf("Flag: %d\n", cur_ctl_flag);
        if (cur_ctl_flag == 1){
            cur_ctl_flag = 0;
            updateCurrent();
            //CurrentControl(TARGET_CUR);
//            UARTprintf("Flag\n");
        }
        if (vel_ctl_flag == 1){
            vel_ctl_flag = 0;
            updateAngle();
            updateVelocity();
            VelocityControl(TARGET_VELO);
        }
        if (pos_ctl_flag == 1){
//            PositionControl(TARGET_ANGLE);
            pos_ctl_flag = 0;
        }
    }
}
