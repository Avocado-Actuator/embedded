/*
 * rs485.c
 *
 *  Created on: Apr 21, 2018
 *      Author: Ryan
 */

#include "rs485.h"

uint8_t ADDRESS, BRAIN_ADDRESS;

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
    addrmask = 0b00001111;
    cmdmask = 0b00010000; // when you filter message with this, 1 is SET and 0 is GET
    posmask = 0b00100000;
    curmask = 0b01000000;
    velmask = 0b10000000;
    heartmask = 0b11110000;
    BRAIN_ADDRESS = 0x00;
    UARTprintf("RS485 initialized\n");
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
    // TODO: implement construction of prefix bytes by ORing masks
    // TODO: create data flags avocados can flip for the brain
    // Add CRC byte to message
    uint8_t crc = crc8(0, pui8Buffer, ui32Count);
    // Construct address/flags byte prefix
    uint8_t prefix = 0x00 | BRAIN_ADDRESS;
    // Set transceiver rx/tx pin high to send
    UARTSetWrite();
    bool space = true;
    // Send the CRC for error-checking
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, prefix);
    while (!space){
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, prefix);
    }
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
    // Send the CRC for error-checking
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, crc);
    while (!space){
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, crc);
    }
    // Send the stopbyte
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOPBYTE);
    while (!space) {
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOPBYTE);
    }
}

void heartBeat(){
    uint8_t beat = 0;
    //room for four flags:
    // limitingflag if the brain is asking for some performance level
    // but the avocado has limited the performance due to current/temp safety levels
    // TODO: come up with things an avocado might want to tell the brain
    beat = BRAIN_ADDRESS; // OR other flags here for status updates to the brain
    bool space = true;
    uint8_t crc = crc8(0, &beat, 1);
    UARTSetWrite();
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, beat);
    while (!space){
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, beat);
    }
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, crc);
    while (!space){
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, crc);
    }
    // Send the stopbyte
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOPBYTE);
    while (!space) {
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOPBYTE);
    }
}

/**
 * Returns string corresponding to given enum value.
 *
 * @param par - enum value whose name to return
 * @return the name of the enum value given
 */
const char* getParameterName(enum Parameter par) {
    switch(par) {
        case Pos: return "Pos";
        case Vel: return "Vel";
        case Cur: return "Cur";
        default: return "NOP";
    }
}

/**
 * Returns string corresponding to given enum value.
 *
 * @param par - enum value whose name to return
 * @return the name of the enum value given
 */
const char* getCommandName(enum Command cmd) {
    switch(cmd) {
        case Get: return "Get";
        case Set: return "Set";
        default: return "NOP";
    }
}

/**
 * Sends value of given parameter on UART.
 *
 * @param par - parameter to send
 */
void
sendData(enum Parameter par) {
    UARTprintf("\nin sendData\n");

    float value;
    switch(par) {
        case Pos: UARTprintf("Current pos: "); UARTPrintFloat(getAngle(), false); value = getAngle(); break;
        case Vel: UARTprintf("Current vel: "); UARTPrintFloat(getVelocity(), false); value = getVelocity(); break;
        case Cur: UARTprintf("Current current: "); UARTPrintFloat(getCurrent(), false); value = getCurrent(); break;
        case Tmp: UARTprintf("Current temperature: "); UARTPrintFloat(getTemp(), false); value = getTemp(); break;
        default: UARTprintf("Asked for invalid parameter, aborting"); return;
    }

    uint8_t* str[4] = &value;
    UARTprintf("\nMessage %s", str);
    UARTprintf("Length: %d\n", strlen(str));

    UARTSend(str, 4);
}

/**
 * Sets given parameter to given value.
 *
 * @param par - parameter to set
 * @param value - value to set parameter to
 */
void
setData(enum Parameter par, float* value) {
    UARTprintf("\nin setData\n");

    UARTprintf("Target value: %d\n", *value);

    switch(par) {
        case Pos: setTargetAngle(*value); UARTprintf("New value: "); UARTPrintFloat(getTargetAngle(), false); break;
        case Vel: setTargetVelocity(*value); UARTprintf("New value: "); UARTPrintFloat(getTargetVelocity(), false); break;
        case Cur: setTargetCurrent(*value); UARTprintf("New value: "); UARTPrintFloat(getTargetCurrent(), false); break;
        case Tmp: UARTprintf("Invalid set, user tried to set temperature"); return;
        default: UARTprintf("Tried to set invaliad parameter, aborting"); return;
    }

    UARTSend("Successfully set", 16);
}

/**
 * Takes actions on message as appropriate.
 *
 * If address does not match our own, bail out and send message on.
 *
 *
 * @param buffer - pointer to the message
 * @param length - the length of the message
 * @param verbose - if true print to console for debugging
 * @param echo - if true simply echo the message, can also be helpful for debugging
 * @return if we successfully handled a message meant for us
 */
bool
handleUART(char* buffer, uint32_t length, bool verbose, bool echo) {
    if(echo) {
        UARTSend((uint8_t *)buffer, length);
    } else {
        UARTprintf("\n\nText: %s\n\n", buffer);

        // get address
        char tempaddr = buffer[0] & addrmask;
        if(verbose) UARTprintf("Address: %d\n", tempaddr);

        if(tempaddr != ADDRESS) {
            if(verbose) UARTprintf("Not my address, abort");
            return false;
        }

        if (buffer[0] & heartmask == 0) {
            // called for heartbeat messages, where brain sends just address and 0's in prefix byte
            heartBeat();
        }

        enum Command type = buffer[0] & cmdmask ? Set : Get;
        if(verbose) UARTprintf("Command: %s\n", getCommandName(type));

        // get parameter - { pos, vel, cur }
        enum Parameter par;

        if (buffer[0] & posmask) par = Pos;
        else if (buffer[0] & velmask) par = Vel;
        else if (buffer[0] & curmask) par = Cur;
        else {
            if(verbose) UARTprintf("No parameter specified, abort");
            return false;
        }

        if(verbose) UARTprintf("Parameter: %s\n", getParameterName(par));

        if(type == Set) {
            // if set command then get parameter value to set to
            // I'm really so sorry about this notation. Since the first byte is the addr/command/parameter,
            // if the cmd is Set then the next entity is a value. This value MUST be a single float
            // which takes the next four bytes of buffer (followed by STOPBYTE)
            float* value = (float *)(&buffer[1]);
            if(verbose) UARTPrintFloat("Set val: %d\n", *value);
            setData(par, value);
        } else {
            sendData(par);
        }
    }

    return true;
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
    char recv[8] = "";
    uint32_t ind = 0;
    char curr = ROM_UARTCharGet(UART7_BASE);
    // Loop while there are characters in the receive FIFO.
    while(curr != STOPBYTE && ind < 8)
    {
        recv[ind] = curr;
        ind++;
        curr = ROM_UARTCharGet(UART7_BASE);
    }

    // keep stop byte for tokenizing
    recv[ind] = curr;
    ind++;

    /*uint8_t crcin = recv[ind-1];
    if (crc8(0, (uint8_t *)recv, ind - 1) != crcin){
        // ********** ERROR ***********
        // Handle corrupted message
    }*/

    // handle given message
    handleUART(recv, ind, true, false);
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

uint8_t UARTGetAddress() { return ADDRESS; }
void UARTSetAddress(uint8_t addr) { ADDRESS = addr; }

void UARTPrintFloat(float val, bool verbose) {
    char str[80]; // pretty arbitrarily chosen
    sprintf(str, "%f", val);
    verbose
        ? UARTprintf("val, length: %s, %d\n", str, strlen(str))
        : UARTprintf("%s\n", str);
}
