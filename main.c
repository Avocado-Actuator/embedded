\/*
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
        ROM_UARTCharPutNonBlocking(UART0_BASE,
                                   ROM_UARTCharGetNonBlocking(UART0_BASE));

        zeroPosition();

    }
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

    PWMoutput(0);
    //disableDriver();

    int dut=0;
    while(1)
    {
        // Check the busy flag in the uart7 register. If not busy, set transceiver pin low

        if (UARTReady()){
            UARTSetRead();
        }

        // Check if button is pushed for zero position
        /*uint8_t ui8Buttons = ButtonsPoll(0, 0);
        if (ui8Buttons & USR_SW1){
            zeroPosition();
        }
        */

        if (time_flag_motor==1){
            time_flag_motor=0;
            updateAngle();
            updateVelocity();
            VelocityControl(5);

            // VelocityControl();
            // Get current data
        }

        if (time_flag_console == 1){
            time_flag_console = 0;

            /*
            if(f==1){
                dut+=5;
                if(dut>=60){
                    f=0;
                }
            }
            else{
                dut-=5;
                if(dut<=0){
                    f=1;
                }

            }
            */
            //UARTprintf("fre: %d\n", 20);
            //UARTprintf("getPWM: %d\n\n",(int)getPWM());
            //updateTemp();
            //UARTprintf("Temp: %d\n", (int)getTemp());
            //updateCurrent();
            //UARTprintf("\nCurrent: %d\n", (int)getCurrent());
            //UARTprintf("Angle: %d\n", (int)getAngle());
            UARTprintf("Vel: %d\n", (int)getVelocity());
        }


    }
}
