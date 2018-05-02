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
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOPBYTE);
    while (!space) {
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOPBYTE);
    }
}

/**
 * Extracts token up to given end byte, placing into given token pointer.
 *
 * @param buffer - pointer to start of string from which to extract token
 * @param token - pointer to fill with token
 * @param endByte - byte to extract until
 * @param maxLen - maximum possible length, important in case message is
 *   corrupted or end byte does not exist in string
 * @return pointer to start of next token
 */
char*
getToken(char* buffer, char* token, char endByte, int maxLen) {
    uint32_t counter = 0;
    // index of filling token buffer
    uint32_t token_index = 0;
    while(*buffer != endByte && counter < maxLen) {
        token[token_index] = *buffer;

        ++counter;
        ++token_index;
        ++buffer;
    }

    // terminate string with null character
    token[token_index] = '\0';
    // advance buffer past end byte
    return ++buffer;
}

/**
 * Calculates number of characters between two pointers.
 *
 * @param first - the first pointer, address must be less than second
 * @param second - the second pointer, address must be greater than first
 * @return number of characters between pointers
 */
uint32_t distBetween(char* first, char* second) {
    return (second - first)*sizeof(*first);
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
 * Sends value of given parameter on UART.
 *
 * @param par - parameter to send
 */
void
sendData(enum Parameter par) {
    uint32_t value;
    switch(par) {
        case Pos: value = 12; break;
        case Vel: value = getVelocity(); break;
        case Cur: value = 321; break;
        default: UARTprintf("Asked for invalid parameter, aborting"); return;
    }

    char str[40];
    sprintf(str, "%d", value);
    UARTprintf("In sendData\n");
    UARTprintf("Converted string: %s\n", str);
    UARTprintf("Length: %d\n", strlen(str));

    UARTSend((uint8_t *) str, strlen(str));
}

/**
 * Sets given parameter to given value.
 *
 * @param par - parameter to set
 * @param value - value to set parameter to
 */
void
setData(enum Parameter par, char* value) {
    uint32_t converted = (uint32_t) strtol(value, NULL, 10);
    UARTprintf("In setData\n");
    UARTprintf("Converted value: %d\n", converted);

    switch(par) {
        case Pos: converted = 12; break;
        case Vel: converted = getVelocity(); break;
        case Cur: converted = 321; break;
        default: UARTprintf("Tried to set invaliad parameter, aborting"); return;
    }

    // UARTSend((uint8_t *) str, strlen(str));
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
        char tok[40] = "";
        // initially tokenize on spaces
        char endByte = ' ';

        // MANUALLY SETTING ADDRESS
        ADDRESS = 1;

        // get address
        char* iter = getToken(buffer, tok, ' ', 40);
        if(verbose) UARTprintf("Address: %s\n", tok);

        if(strtol(tok, NULL, 10) != ADDRESS) {
            if(verbose) UARTprintf("Not my address, abort");
            return false;
        }

        // trailing pointer to end of last token
        // can be used to calculate token length
        // char* trail = buffer; // reset to iter on subsequent uses
        // uint32_t token_length = distBetween(trail, iter) - 1;

        // get either "get" or "set"
        iter = getToken(iter, tok, ' ', 40);
        if(verbose) UARTprintf("Command: %s\n", tok);

        enum Command type = strcmp(tok, "get") == 0 ? Get : Set;
        // if get then next token is last and ends at STOPBYTE
        endByte = type == Get ? STOPBYTE : ' ';

        // get parameter - { pos, vel, cur }
        iter = getToken(iter, tok, endByte, 40);
        if(verbose) UARTprintf("Parameter: %s\n", tok);

        enum Parameter par = strcmp(tok, "pos") == 0 ? Pos
                : strcmp(tok, "vel") == 0 ? Vel : Cur;

        if(type == Set) {
            // if set command then get parameter value to set to
            iter = getToken(iter, tok, STOPBYTE, 40);
            if(verbose) UARTprintf("Set val: %s\n", tok);
            setData(par, tok);
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
    char recv[40] = "";
    uint32_t ind = 0;
    char curr = ROM_UARTCharGet(UART7_BASE);
    // Loop while there are characters in the receive FIFO.
    while(curr != STOPBYTE && ind < 40)
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

void UARTSetAddress(long addr){
    ADDRESS = addr;
}

long UARTGetAddress(){
    return ADDRESS;
}

void UARTPrintFloat(float val, bool printLength) {
    char str[80]; // pretty arbitrarily chosen
    sprintf(str, "%f", val);
    UARTprintf("%s\n", str);
    if(printLength) UARTprintf("Length: %d\n", strlen(str));
}
