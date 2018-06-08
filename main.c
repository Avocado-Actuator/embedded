/*
 * Main file for the Avocado embedded code
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h" // NEW INCLUDE!!!!
#include "inc/hw_sysctl.h" // NEW INCLUDE!!!
#include "inc/hw_types.h" // NEW INCLUDE!!!
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "isense.h" // NEW INCLUDE!!!
#include "utils/uartstdio.h" // NEW INCLUDE!!!
#include "rs485.h" // NEW INCLUDE!!!
#include "reflectance.h" // NEW INCLUDE!!!
#include "buttons.h" // NEW INCLUDE!!!
#include "mag_encoder.h" // NEW INCLUDE!!!
#include "motor.h"
#include "temp.h"

// System clock rate in Hz.
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


//*****************************************************************************
//
// Initializes UART0 for console output using UARTStdio
//
//*****************************************************************************
void
ConsoleInit(void)
{
    // Enable GPIO port A which is used for UART0 pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    // Configure the pin muxing for UART0 functions on port A0 and A1.
    // This step is not necessary if your part does not support pin muxing.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    // Enable UART0 so that we can configure the clock.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    // Select the alternate (UART) function for these pins.
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, 16000000);

   ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
   ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);



   //
   // Enable the UART interrupt.
   //
   ROM_IntEnable(INT_UART0);
   ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

}

//*****************************************************************************
void
UARTIntHandler0(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        char ch=ROM_UARTCharGetNonBlocking(UART0_BASE);
        ROM_UARTCharPutNonBlocking(UART0_BASE,ch);
        TARGET_ANGLE=(int)(ch-'0')*36;
//        if(flag==1){
//            flag=0;
//        }
//        else{
//            flag=1;
//        }
        //zeroPosition();

    }
}


//*****************************************************************************
// Handler of timer interrupts, set a clock for program
//
// input: None
//
// return: None
//*****************************************************************************
int timer0_cnt = 0;
volatile uint32_t count=0;
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
//    if(flag){
//        PositionControl(TARGET);
//    }
//    else{
//        PWMoutput(0);
//    }

}

void Timer1IntHandler(void){
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
//    updateAngle();
//    UARTprintf("Angle:%d\n\n",(int)getAngle());
    //updateCurrent();
    //UARTprintf("Current: %d\n", (int)getCurrent());
//    UARTprintf("duty: %d\n\n", (int)duty);

    UARTprintf("count: %d\n", (int)count);
    count=0;
//
    UARTprintf("Target Angle: %d\n", (int)TARGET_ANGLE);
    UARTprintf("Angle: %d\n", (int)getAngle());
    UARTprintf("Target Velocity: %d\n", (int)TARGET_VELO);
    UARTprintf("Velocity: %d\n", (int)getVelocity());
    UARTprintf("Target Current: %d\n", (int)TARGET_CUR);
    UARTprintf("Current: %d\n", (int)getCurrent());
    UARTprintf("outputPWN: %d\n\n", (int)duty);

    updateTemp();
    UARTprintf("Temp: %d\n", (int)getTemp());
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
    //RSInit(g_ui32SysClock); // Initialize the RS485 link

    EncoderInit(g_ui32SysClock);
    ReflectInit();
    ButtonsInit();
    TempInit(g_ui32SysClock);
    MotorInit(g_ui32SysClock);
    CurrentSenseInit();
    TimerInit(g_ui32SysClock);

    UARTprintf("Initialized\n\n");


    PWMoutput(0);
    //disableDriver();


    while(1)
    {
        if (cur_ctl_flag == 1){
            cur_ctl_flag = 0;
//            count++;
            updateCurrent();
//            CurrentControl(TARGET_CUR);
        }
        if (vel_ctl_flag == 1){
            vel_ctl_flag = 0;
            count++;
            updateAngle();
            updateVelocity();
            PositionControl(TARGET_ANGLE);
//            VelocityControl(0);
//            PositionControl(TARGET_ANGLE);
        }

    }
}
