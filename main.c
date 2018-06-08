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

#include "timer.h"
#include "isense.h"
#include "comms.h"
#include "reflectance.h"
#include "mag_encoder.h"
#include "motor.h"
#include "temp.h"

// System clock rate in Hz.
uint32_t g_ui32SysClock;

/**
 * Called if driver library encounters an error
 */
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) { }
#endif

//*****************************************************************************
// SUPER IMPORTANT, CCS IS JANK, NEED TO INCREASE STACK SIZE MANUALLY IN ORDER
// TO USE SPRINTF, GO TO Build -> ARM Linker -> Basic Options IN PROJECT
// PROPERTIES SET C SYSTEM STACK SIZE TO 65536
//*****************************************************************************

/**
 * Main loop
 *
 * @return doesn't really matter
 */
int main(void) {

    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = MAP_SysCtlClockFreqSet(
        (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
        120000000);

    ROM_IntMasterEnable(); // Enable processor interrupts.

    ConsoleInit(); // Initialized UART0 for console output using UARTStdio
    CommsInit(g_ui32SysClock);
    UARTprintf("\n\nTiva has turned on...\n");

    UARTprintf("Initializing...\n");

    EncoderInit(g_ui32SysClock);
    ReflectInit();
    TempInit(g_ui32SysClock);
    MotorInit(g_ui32SysClock);
    CurrentSenseInit();
    TimerInit(g_ui32SysClock);

    UARTprintf("Initialized\n\n");

    PWMoutput(0);

    while(1) {
        if (cur_ctl_flag == 1){
            cur_ctl_flag = 0;
            updateCurrent();
            // CurrentControl(TARGET_CUR);
        }

        if (vel_ctl_flag == 1){
            vel_ctl_flag = 0;
            updateAngle();
            updateVelocity();
            // PositionControl(TARGET_ANGLE);
            VelocityControl(TARGET_VELO);
        }

    }
}
