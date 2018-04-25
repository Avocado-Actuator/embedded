/*
 * rs485.c
 *
 *  Created on: Apr 21, 2018
 *      Author: Ryan
 */

#include "rs485.h"

void
RSInit(uint32_t g_ui32SysClock){
    // Copy over the clock created in main
    uartSysClock = g_ui32SysClock;
    // Enable the peripherals used
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPinConfigure(GPIO_PC4_U7RX);
    GPIOPinConfigure(GPIO_PC5_U7TX);
    ROM_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    // Enable GPIO port C pin 6 as the RS-485 transceiver rx/tx pin
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);
    // enable tied pin as input to read output of enable pin
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_7);
    // Write transceiver enable pin low for listening
    UARTSetRead();
    // Configure the UART for 115,200, 8-N-1 operation.
    ROM_UARTConfigSetExpClk(UART7_BASE, uartSysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART7);
    ROM_UARTIntEnable(UART7_BASE, UART_INT_RX | UART_INT_RT);
    return;
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    // Add CRC byte to message
    uint8_t crc = crc8(0, pui8Buffer, ui32Count);
    // Set transceiver rx/tx pin high to send
    UARTSetWrite();
    bool space = true;
    // Loop while there are more characters to send.
    while(ui32Count--)
    {
        // Write the next character to the UART.
        // putchar returns false if the send FIFO is full
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, *pui8Buffer);
        // If send FIFO is full, wait until we can put the char in
        while (!space){
            space = ROM_UARTCharPutNonBlocking(UART7_BASE, *pui8Buffer);
        }
        *pui8Buffer++;
    }
    /*space = ROM_UARTCharPutNonBlocking(UART7_BASE, crc);
    // If send FIFO is full, wait until we can put the char in
    while (!space){
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, crc);
    }*/
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    uint32_t ui32Status;
    // Get the interrupt status.
    ui32Status = ROM_UARTIntStatus(UART7_BASE, true);
    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART7_BASE, ui32Status);
    // Initialize recv buffer
    char recv[40] = "";
    uint32_t ind = 0;
    // Loop while there are characters in the receive FIFO.
    while(ROM_UARTCharsAvail(UART7_BASE))
    {
        recv[ind] = ROM_UARTCharGet(UART7_BASE);
        ind++;
    }
    /*uint8_t crcin = recv[ind-1];
    if (crc8(0, (uint8_t *)recv, ind - 1) != crcin){
        // ********** ERROR ***********
        // Handle corrupted message
    }*/
    // Send back everything that we received in that batch
    UARTSend((uint8_t *)recv, ind);
}

bool
UARTReady(){
    return GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7) && !UARTBusy(UART7_BASE);
}

void
UARTSetRead(){
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0); // Set transceiver rx/tx pin low for read mode
    return;
}

void
UARTSetWrite(){
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6); // Set transceiver rx/tx pin low for read mode
    return;
}

void UARTSetAddress(uint8_t addr){
    uartAddress = addr;
}

uint8_t UARTGetAddress(){
    return uartAddress;
}
