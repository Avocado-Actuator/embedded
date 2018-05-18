/*
 * rs485.c
 *
 *  Created on: Apr 21, 2018
 *      Author: Ryan
 */

#include "rs485.h"

uint8_t ADDRESS, BRAIN_ADDRESS, BROADCASTADDR, ADDRSETADDR;

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
    MAX_PARAMETER_VALUE = 9;
    cmdmask = 0b10000000; // when you filter message with this, 1 is SET and 0 is GET
    parmask = 0b00000111; // this gives you just the parameter selector bits
    BRAIN_ADDRESS = 0x00;
    BROADCASTADDR = 0xFF;
    ADDRSETADDR = 0XFE;
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
    uint8_t crc = crc8(0, &msg, len);
    // Set transceiver rx/tx pin high to send
    UARTSetWrite();
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
        case Pos: UARTprintf("Current pos: "); UARTPrintFloat(getAngle(), false); value.f = getAngle(); setStatus(COMMAND_SUCCESS); break;
        case Vel: UARTprintf("Current vel: "); UARTPrintFloat(getVelocity(), false); value.f = getVelocity(); setStatus(COMMAND_SUCCESS); break;
        case Cur: UARTprintf("Current current: "); UARTPrintFloat(getCurrent(), false); value.f = getCurrent(); setStatus(COMMAND_SUCCESS); break;
        case Tmp: UARTprintf("Current temperature: "); UARTPrintFloat(getTemp(), false); value.f = getTemp(); setStatus(COMMAND_SUCCESS); break;
        case MaxCur: UARTprintf("Max Current Setting: "); UARTPrintFloat(getMaxCurrent(), false); value.f = getMaxCurrent(); setStatus(COMMAND_SUCCESS); break;
        case EStop: UARTprintf("Emergency Stop Behaviour: "); UARTprintf(*(getEStop()), false); value.bytes[0] = *(getEStop()); setStatus(COMMAND_SUCCESS); UARTSend(value.bytes[0], 1); return;
        case Status: UARTprintf("Status Register: "); UARTprintf(*(getStatus()), false); value.bytes[0] = *(getStatus()); setStatus(COMMAND_SUCCESS); UARTSend(value.bytes[0], 1); return;
        default: UARTprintf("Asked for invalid parameter, aborting"); setStatus(COMMAND_FAILURE); break;
    }
    if (!(*(getStatus()) & ~COMMAND_FAILURE)){ // true if command failed
        UARTSend(getStatus(), 1);
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
        case Pos: setTargetAngle(value->f); UARTprintf("New value: "); UARTPrintFloat(getTargetAngle(), false); setStatus(COMMAND_SUCCESS); break;
        case Vel: setTargetVelocity(value->f); UARTprintf("New value: "); UARTPrintFloat(getTargetVelocity(), false); setStatus(COMMAND_SUCCESS); break;
        case Cur: setTargetCurrent(value->f); UARTprintf("New value: "); UARTPrintFloat(getTargetCurrent(), false); setStatus(COMMAND_SUCCESS); break;
        case Adr: UARTSetAddress(value->bytes[0]); UARTprintf("New value: "); UARTprintf(UARTGetAddress(), false); setStatus(COMMAND_SUCCESS); break;
        case MaxCur: setMaxCurrent(value->f); UARTprintf("New value: "); UARTPrintFloat(getMaxCurrent(), false); setStatus(COMMAND_SUCCESS); break;
        case EStop: setEStop(value->bytes[0]); UARTprintf("New value: "); UARTprintf(*(getEStop()), false); setStatus(COMMAND_SUCCESS); break;
        case Tmp: UARTprintf("Invalid set, user tried to set temperature"); setStatus(COMMAND_FAILURE); break;
        default: UARTprintf("Tried to set invaliad parameter, aborting"); setStatus(COMMAND_FAILURE); break;
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
handleUART(char* buffer, uint32_t length, bool verbose, bool echo) {
    if(echo) {
        UARTSend((uint8_t *)buffer, length);
        //UARTprintf("\n\nText: %s\n\n", buffer);
        int i = 0;
        for (i = 0; i < length; i++){
            UARTprintf("Text[%d]: %s\n", i, buffer[i]);
        }
    } else {
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
    char recv[10] = "";
    uint32_t ind = 0;
    char curr = ROM_UARTCharGet(UART7_BASE);
    // Loop while there are characters in the receive FIFO.
    while(curr != STOPBYTE && ind < 10)
    {
        recv[ind] = curr;
        ind++;
        curr = ROM_UARTCharGet(UART7_BASE);
    }

    // keep stop byte for tokenizing
    recv[ind] = curr;
    ind++;

    /*uint8_t crcin = recv[ind-2];
    if (crc8(0, (uint8_t *)recv, ind-2) != crcin){
        // ********** ERROR ***********
        // Handle corrupted message
    }*/

    // handle given message
    if (handleUART(recv, ind, true, true)){
        UARTSend(getStatus(), 1);
    }
    return;
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
    char str[16]; // pretty arbitrarily chosen
    sprintf(str, "%f", val);
    verbose
        ? UARTprintf("val, length: %s, %d\n", str, strlen(str))
        : UARTprintf("%s\n", str);
}
