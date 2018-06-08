#include "comms.h"

// ADDRESSES
uint8_t ADDR, BRAIN_ADDR, BROADCAST_ADDR, ADDRSET_ADDR;
// COUNTERS
uint8_t message_counter, panic_counter;
// INDICES
uint8_t ADDR_IND, ADDRSET_IND, ID_IND, COM_IND, PAR_VAL_OFFSET;

uint16_t NO_RESPONSE;


uint8_t recv[10];

// <<<<<<<<<<<<<<<>>>>>>>>>>>>>>
// <<<<<<<<<<<< INITS >>>>>>>>>>
// <<<<<<<<<<<<<<<>>>>>>>>>>>>>>

/**
 * Initializes UART7 for communication between boards
 *
 * @param g_ui32SysClock - system clock to sync with
 */
void CommsInit(uint32_t g_ui32SysClock){
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
    // Configure the UART for 115,200, 8-N-1 operation.
    ROM_UARTConfigSetExpClk(UART7_BASE, g_ui32SysClock, 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART7);
    ROM_UARTIntEnable(UART7_BASE, UART_INT_RX | UART_INT_RT);

    recvIndex = 0;
    MAX_PAR_VAL = 9;
    STOP_BYTE = '!';

    BRAIN_ADDR = 0x0;
    ADDR = 0x1;
    BROADCAST_ADDR = 0xFF;
    ADDRSET_ADDR = 0xFE;

    ADDR_IND        = 0;
    ADDRSET_IND     = 2;
    ID_IND          = 1;
    COM_IND         = 2;
    PAR_VAL_OFFSET  = 3;

    CMD_MASK = 0b10000000; // 1 is SET and 0 is GET
    PAR_MASK = 0b00000111; // gives just parameter selector bits

    // matching flags should be inverse
    ESTOP_HOLD      = 0b00000001;
    ESTOP_KILL      = 0b11111110;

    COMMAND_SUCCESS = 0b00000010;
    COMMAND_FAILURE = 0b11111101;

    OUTPUT_LIMITING = 0b00000100;
    OUTPUT_FREE     = 0b11111011;

    STATUS          = 0b00000000;

    NO_RESPONSE = 65535;

    message_counter = 0;
    panic_counter = 0;

    UARTprintf("Communication initialized\n");
}

/**
 * Initializes UART0 for console output using UARTStdio
 */
void ConsoleInit(void) {
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
    UARTStdioConfig(0, 9600, 16000000);

    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTprintf("Console initialized\n");
}

// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// <<<<<<<<<<< UTILITIES >>>>>>>>>>
// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>

/**
 * Sets new status
 *
 * @param newflag - new status flag
 */
void setStatus(uint8_t newflag) { STATUS |= newflag; }
void clearStatus(uint8_t newflag) { STATUS &= newflag; }
uint8_t getStatus() { return STATUS; }

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
        case MaxCur: return "MaxCur"; // max current
        case Status: return "Status"; // status register
        case EStop: return "EStop"; // emergency stop behavior
        case Adr: return "Adr";
        default: return "NOP";
    }
}

/**
 * Get personal address
 *
 * @return our address
 */
uint8_t getAddress() { return ADDR; }

/**
 * Set personal address
 *
 * @param our new address
 */
void setAddress(uint8_t addr) { ADDR = addr; }

/**
 * Prints given float
 *
 * @param val - float to print
 * @param verbose - how descriptive to be in printing
 */
void UARTPrintFloat(float val, bool verbose) {
    char str[100]; // pretty arbitrarily chosen
    sprintf(str, "%f", val);
    verbose
        ? UARTprintf("val, length: %s, %d\n", str, strlen(str))
        : UARTprintf("%s\n", str);
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
    space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOP_BYTE);
    while (!space) {
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, STOP_BYTE);
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

    // MANUALLY SETTING BRAIN ADDR FOR NOW
    BRAIN_ADDR = 0;

    char str[40];
    sprintf(str, "%d %f", BRAIN_ADDR, value);
    UARTprintf("\nMessage %s", str);
    UARTPrintFloat(value, true);

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
    UARTprintf("\nin setData\n");

    float converted = strtof(value, NULL);
    UARTPrintFloat(converted, false);

    switch(par) {
        case Pos: setTargetAngle(converted); UARTprintf("New value: "); UARTPrintFloat(getTargetAngle(), false); break;
        case Vel: setTargetVelocity(converted); UARTprintf("New value: "); UARTPrintFloat(getTargetVelocity(), false); break;
        case Cur: setTargetCurrent(converted); UARTprintf("New value: "); UARTPrintFloat(getTargetCurrent(), false); break;
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
        char tok[40] = "";
        // initially tokenize on spaces
        char endByte = ' ';

        // MANUALLY SETTING ADDR
        ADDR = 1;

        // get address
        char* iter; // = getToken(buffer, tok, ' ', 40);
        if(verbose) UARTprintf("Address: %s\n", tok);

        if(((uint8_t) strtol(tok, NULL, 10)) != ADDR) {
            if(verbose) UARTprintf("Not my address, abort");
            return false;
        }

        // trailing pointer to end of last token
        // can be used to calculate token length
        // char* trail = buffer; // reset to iter on subsequent uses
        // uint32_t token_length = distBetween(trail, iter) - 1;

        // get either "get" or "set"
        // iter = getToken(iter, tok, ' ', 40);
        if(verbose) UARTprintf("Command: %s\n", tok);

        enum Command type = strcmp(tok, "get") == 0 ? Get : Set;
        // if get then next token is last and ends at STOP_BYTE
        endByte = type == Get ? STOP_BYTE : ' ';

        // get parameter - { pos, vel, cur }
        // iter = getToken(iter, tok, endByte, 40);
        if(verbose) UARTprintf("Parameter: %s\n", tok);

        enum Parameter par =
                strcmp(tok, "pos") == 0 ? Pos
                : strcmp(tok, "vel") == 0 ? Vel
                : strcmp(tok, "cur") == 0 ? Cur : Tmp;

        if(type == Set) {
            // if set command then get parameter value to set to
            // iter = getToken(iter, tok, STOP_BYTE, 40);
            if(verbose) UARTprintf("Set val: %s\n", tok);
            setData(par, tok);
        } else {
            sendData(par);
        }
    }

    return true;
}

// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// <<<<<<<<<<<< HANDLERS >>>>>>>>>>
// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>

/**
 * Console interrupt handler
 */
void ConsoleIntHandler(void) {
    // Get the interrupt status.
    uint32_t ui32Status = ROM_UARTIntStatus(UART0_BASE, true);
    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART0_BASE, ui32Status);

    // Loop while there are characters in the receive FIFO.
    while(ROM_UARTCharsAvail(UART0_BASE)) {
        // Read the next character from the UART and write it back to the UART.
        ROM_UARTCharPutNonBlocking(UART7_BASE, ROM_UARTCharGetNonBlocking(UART0_BASE));
        // Blink the LED to show a character transfer is occuring.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        SysCtlDelay(uartSysClock / (1000 * 3));
        // Turn off the LED
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
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
    char recv[40] = "";
    uint32_t ind = 0;
    char curr = ROM_UARTCharGet(UART7_BASE);
    // Loop while there are characters in the receive FIFO.
    while(curr != STOP_BYTE && ind < 40)
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
