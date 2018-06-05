/*
 * rs485.c
 *
 *  Created on: Apr 21, 2018
 *      Author: Ryan
 */

#include "rs485.h"

uint8_t ADDRESS, BRAIN_ADDRESS, BROADCASTADDR, ADDRSETADDR;
uint8_t recv[10];

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
    //UARTSetRead();
    // Configure the UART for 115,200, 8-N-1 operation.
    ROM_UARTConfigSetExpClk(UART7_BASE, uartSysClock, 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART7);
    ROM_UARTIntEnable(UART7_BASE, UART_INT_RX | UART_INT_RT);
    MAX_PARAMETER_VALUE = 9;
    cmdmask = 0b10000000; // when you filter message with this, 1 is SET and 0 is GET
    parmask = 0b00000111; // this gives you just the parameter selector bits
    BRAIN_ADDRESS = 0x00;
    BROADCASTADDR = 0xFF;
    ADDRSETADDR = 0XFE;
    recvIndex = 0;
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
    uint8_t msg[8];
    msg[0] = BRAIN_ADDRESS;
    int i, len;
    len = 1;
    for (i = 0; i < ui32Count; i++){
        msg[i+1] = pui8Buffer[i];
        len++;
    }
    // Add CRC byte to message
    uint8_t crc = crc8(0, (const unsigned char*) &msg, len);
    // Set transceiver rx/tx pin high to send
    //UARTSetWrite();


    bool space = true;
    // Loop while there are more characters to send.
    for (i = 0; i < len; i++) {
        // Write the next character to the UART.
        // putchar returns false if the send FIFO is full
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, msg[i]);
        // If send FIFO is full, wait until we can put the char in
        while (!space){
            space = ROM_UARTCharPutNonBlocking(UART7_BASE, msg[i]);
        }
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
        case Tmp: return "Tmp";
        case MaxCur: return "MaxCur"; // Max Current
        case Status: return "Status"; // status register
        case EStop: return "EStop"; // Emergency stop behavior, kill motor or hold position
        case Adr: return "Adr";
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
    union Flyte value;
    switch(par) {
        case Pos: {
            UARTprintf("Current pos: ");
            UARTPrintFloat(getAngle(), false);
            value.f = getAngle();
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case Vel: {
            UARTprintf("Current vel: ");
            UARTPrintFloat(getVelocity(), false);
            value.f = getVelocity();
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Cur: {
            UARTprintf("Current current: ");
            UARTPrintFloat(getCurrent(), false);
            value.f = getCurrent();
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Tmp: {
            UARTprintf("Current temperature: ");
            UARTPrintFloat(getTemp(), false);
            value.f = getTemp();
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case MaxCur: {
            UARTprintf("Max Current Setting: ");
            UARTPrintFloat(getMaxCurrent(), false);
            value.f = getMaxCurrent();
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case EStop: {
            UARTprintf("Emergency Stop Behaviour: ");
            uint8_t temp = getEStop();
            UARTprintf((const char*) &temp, false);
            value.bytes[0] = temp;
            setStatus(COMMAND_SUCCESS);
            uint8_t bytes = value.bytes[0];
            UARTSend((const uint8_t*) &bytes, 1);
            return;
        }

        case Status: {
            UARTprintf("Status Register: ");
            uint8_t temp = getStatus();
            UARTprintf((const char*) &temp, false);
            value.bytes[0] = temp;
            setStatus(COMMAND_SUCCESS);
            uint8_t bytes = value.bytes[0];
            UARTSend((const uint8_t*) &bytes, 1);
            return;
        }

        default: {
            UARTprintf("Asked for invalid parameter, aborting");
             setStatus(COMMAND_FAILURE);
             break;
        }
    }
    if (!(getStatus() & ~COMMAND_FAILURE)){ // true if command failed
        uint8_t temp = getStatus();
        UARTSend(&temp, 1);
    } else {
        UARTSend(value.bytes, 4);
    }
    return;
}

/**
 * Sets given parameter to given value.
 *
 * @param par - parameter to set
 * @param value - value to set parameter to
 */
void
setData(enum Parameter par, union Flyte * value) {
    UARTprintf("\nin setData\n");
    UARTprintf("Target value: %d\n", value->f);
    switch(par) {
        case Pos: {
            setTargetAngle(value->f);
            UARTprintf("New value: ");
            UARTPrintFloat(getTargetAngle(), false);
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case Vel: {
            setTargetVelocity(value->f);
            UARTprintf("New value: ");
            UARTPrintFloat(getTargetVelocity(), false);
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case Cur: {
            setTargetCurrent(value->f);
            UARTprintf("New value: ");
            UARTPrintFloat(getTargetCurrent(), false);
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case Adr: {
            UARTSetAddress(value->bytes[0]);
            UARTprintf("New value: ");
            uint8_t temp = UARTGetAddress();
            UARTprintf((const char*) &temp, false);
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case MaxCur: {
            setMaxCurrent(value->f);
            UARTprintf("New value: ");
            UARTPrintFloat(getMaxCurrent(), false);
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case EStop: {
            setEStop(value->bytes[0]);
            UARTprintf("New value: ");
            uint8_t temp = getEStop();
            UARTprintf((const char*) &temp, false);
            setStatus(COMMAND_SUCCESS);
            break;
        }
        case Tmp: {
            UARTprintf("Invalid set, user tried to set temperature");
            setStatus(COMMAND_FAILURE);
            break;
        }
        default: {
            UARTprintf("Tried to set invaliad parameter, aborting");
            setStatus(COMMAND_FAILURE);
            break;
        }
    }
    return;
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
handleUART(uint8_t* buffer, uint32_t length, bool verbose, bool echo) {
    if(echo) {
        UARTSend((uint8_t *) buffer, length);
        int i;
        for (i = 0; i < length; ++i) {
            UARTprintf("Text[%d]: %s\n", i, buffer[i]);
        }
        return false;
    }

    UARTprintf("\n\nText: %s\n\n", buffer);

    // get address
    char tempaddr = buffer[0];
    if(verbose) UARTprintf("Address: %d\n", tempaddr);

    if(tempaddr == BROADCASTADDR){
        //TODO: Handle heartbeat from brain
        return false;
    } else if (tempaddr == ADDRSETADDR){
        //TODO: Handle address setting command
        setStatus(COMMAND_SUCCESS);
        return true;
    }

    if(tempaddr != UARTGetAddress()) {
        if(verbose) UARTprintf("Not my address, abort");
        return false;
    }

    enum Command type = buffer[1] & cmdmask ? Set : Get;
    if(verbose) UARTprintf("Command: %s\n", getCommandName(type));

    // get parameter - { pos, vel, cur }
    enum Parameter par = buffer[1] & parmask;
    if(par > MAX_PARAMETER_VALUE) {
        if(verbose) UARTprintf("No parameter specified, abort");
        setStatus(COMMAND_FAILURE);
        return true;
    }

    if(verbose) UARTprintf("Parameter: %s\n", getParameterName(par));

    if(type == Set) {
        if (length < 5){
            if(verbose) UARTprintf("No value provided, abort\n");
            setStatus(COMMAND_FAILURE);
            return true;
        }
        // if the cmd is Set then the next entity is a value. This value is either a single float
        // which takes the next four bytes of buffer, or a single byte
        union Flyte setval;
        int i;
        for (i = 0; i < length - 4; i++){
            setval.bytes[i] = buffer[i+2];
        }
        if(verbose) {
            UARTprintf("Set val: ");
            UARTPrintFloat(setval.f, false);
        }
        setData(par, &setval);
        return true;
    } else {
        sendData(par);
        return false;
    }
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    UARTprintf("\nEntering interrupt handler\n");
    // Get the interrupt status.
    uint32_t ui32Status = ROM_UARTIntStatus(UART7_BASE, true);
    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART7_BASE, ui32Status);
    char curr = ROM_UARTCharGet(UART7_BASE);
    UARTprintf("\nGot char: %x\n", curr);
    // reset on received char (set to 1 so we don't immediately fire)
    BUFFER_TIME = 1;
    recv[recvIndex] = curr;
    // tracks length, not index of last element
    ++recvIndex;
    // buffer overflow check
    if(recvIndex > 10) {
        recvIndex = 0;
        UARTprintf("\nBuffer overflow - exceeded max length\n");
    }

    // handle buffer when stopbyte received
    if(curr == STOPBYTE) {
        UARTprintf("\nAbout to handle UART\n");
        if (handleUART(recv, recvIndex, true, true)) {
            uint8_t status = getStatus();
            UARTSend((const uint8_t*) &status, 1);
        }
        recvIndex = 0;

    }
    // Loop while there are characters in the receive FIFO.
//     while(curr != STOPBYTE && ind < 10)
//     {
//         UARTprintf("\nChar: %x\n", curr);
//         recv[ind] = curr;
//         ind++;
// //        curr = ROM_UARTGet(UART7_BASE);
//         curr = ROM_UARTCharGetNonBlocking(UART7_BASE);
//     }

    // keep stop byte for tokenizing
    // recv[ind] = curr;
    // ind++;

    /*uint8_t crcin = recv[ind-2];
    if (crc8(0, (uint8_t *)recv, ind-2) != crcin){
        // ********** ERROR ***********
        // Handle corrupted message
    }*/

    UARTprintf("\nLeaving interrupt handler\n");
}

bool
UARTReady(){
    return GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7) && !UARTBusy(UART7_BASE);
}

void
UARTSetRead(){
    // set transceiver rx/tx pin low for read mode
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
    return;
}

void
UARTSetWrite(){
    // set transceiver rx/tx pin high for read mode
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    return;
}

uint8_t UARTGetAddress() { return ADDRESS; }
void UARTSetAddress(uint8_t addr) { ADDRESS = addr; }

void UARTPrintFloat(float val, bool verbose) {
    char str[16]; // pretty arbitrarily chosen
    sprintf(str, "%f", val);
    verbose
        ? UARTprintf("val, length: %s, %d\n", str, strlen(str))
        : UARTprintf("%s\n", str);
}
