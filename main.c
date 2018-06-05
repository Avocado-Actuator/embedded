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
#include "driverlib/uart.h"
#include "isense.h" // NEW INCLUDE!!!
#include "utils/uartstdio.h" // NEW INCLUDE!!!
#include "utils/uartstdio.c" // NEW INCLUDE!!!
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
    RSInit(g_ui32SysClock); // Initialize the RS485 link
    CurrentSenseInit();
    EncoderInit(g_ui32SysClock);
    ReflectInit();
    ButtonsInit();
    MotorInit(g_ui32SysClock);
    TempInit(g_ui32SysClock);
    UARTprintf("Initialized\n\n");

    UARTSetAddress(0x01);

    //testSpin(5000,80);
    //disableDriver();


    // Loop forever echoing data through the UART.
    int counter = 0;
    int iter_mod = 1000000; // 40,000,000;
    while(1)
    {
        // Check the busy flag in the uart7 register. If not busy, set transceiver pin low
        /*if (UARTReady()){
            UARTSetRead();
            UARTprintf("Setting low");
        }*/

       /*if(counter % iter_mod == 0) {
           UARTprintf("\n\nSending fuck\n");
           UARTSend((const uint8_t*) "fuck\n", 5);
           UARTprintf("Pin is  %d\n", GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7));

           UARTprintf("\nIteration %d\n", counter);
       }*/

       //++counter;

        // Check if button is pushed for zero position
        /*uint8_t ui8Buttons = ButtonsPoll(0, 0);
        if (ui8Buttons & USR_SW1){
            zeroPosition();
        }
        */

        /*if (time_flag_2ms==1){
            time_flag_2ms=0;
            updateAngle();
            if(flag==0) {
                GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,GPIO_PIN_1);
                PositionControl();
            }
            else{
                disableDriver();
                // break;
            }
             updateVelocity();
            // VelocityControl();
            // Get current data
             updateCurrent();
            // UARTprintf("Current: %d\n", getCurrent());
        }

        if (time_flag_1000ms == 1){
            time_flag_1000ms = 0;
            updateTemp();
            UARTprintf("Temp: %d\n", getTemp());
            UARTprintf("\n\nAngle: %d", (int) getAngle());
            UARTprintf("\nTemperature: %d", (int) getTemp());
            UARTprintf("\nCurrent: %d", (int) getCurrent());
            UARTprintf("\nVelocity: %d", (int) getVelocity());
            UARTPrintFloat(getAngle(), 0);
        }


        if (time_flag_10000ms == 1){
            time_flag_10000ms = 0;
            setTargetAngle(((int) getTargetAngle() + 50) % 360);
            UARTprintf("\n\nTarget angle: %d", (int) getTargetAngle());
        }*/

        if(buffer_time_flag == 1) {
            if(recvIndex > 0) {
                UARTprintf("\nBuffer time flag triggered\n");
                UARTprintf("Recv index was > 0, reset\n");
                recvIndex = 0;
            }
            buffer_time_flag = 0;
        }
    }
}
