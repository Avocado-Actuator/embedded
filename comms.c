/**
 * Implementation of UART communication.
 */

#include "comms.h"

// ADDRESSES
uint8_t ADDR, BRAIN_ADDR, BROADCAST_ADDR, ADDRSET_ADDR;
// INDICES
uint8_t ADDR_IND, ADDRSET_IND, ID_IND, COM_IND, PAR_VAL_OFFSET;

uint16_t NO_RESPONSE;

// buffer
uint8_t recv[10];

// <<<<<<<<<<<<<<<>>>>>>>>>>>>>>
// <<<<<<<<<<<< INITS >>>>>>>>>>
// <<<<<<<<<<<<<<<>>>>>>>>>>>>>>

/**
 * Initialize UART7 for communication between boards
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

    MAIN_COMMAND_MODE = ModePos;

    UARTprintf("Communication initialized\n");
}

/**
 * Initialize UART0 for console output using UARTStdio
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
 * Get string corresponding to given enum value.
 *
 * @param cmd - enum value whose name to return
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
 * Get string corresponding to given enum value.
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
 * Print given float
 *
 * @param val - float to print
 * @param verbose - how descriptive to be in printing
 */
void UARTPrintFloat(float val, bool verbose) {
    char str[100]; // pretty arbitrarily chosen
    sprintf(str, "%f", val);
    verbose
        ? UARTprintf("val, length: %s, %d\n", str, strlen(str))
        : UARTprintf("%s", str);
}

// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// <<<<<<<<<<<< ACTIONS >>>>>>>>>>>
// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>

/**
 * Set given parameter to given value.
 *
 * @param par - parameter to set
 * @param value - value to set parameter to
 * @param verbose - if true print to console for debugging
 */
void setData(enum Parameter par, union Flyte * value, bool verbose) {
    switch(par) {
        case Pos: {
            MAIN_COMMAND_MODE = ModePos;
            setTargetAngle(value->f);
            if(verbose) {
                UARTprintf("New target angle: ");
                UARTPrintFloat(getTargetAngle(), false);
            }
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Vel: {
            MAIN_COMMAND_MODE = ModeVel;
            setTargetVelocity(value->f);
            if(verbose) {
                UARTprintf("New target velocity: ");
                UARTPrintFloat(getTargetVelocity(), false);
            }
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Cur: {
            MAIN_COMMAND_MODE = ModeCur;
            setTargetCurrent(value->f);
            if(verbose) {
                UARTprintf("New target current: ");
                UARTPrintFloat(getTargetCurrent(), false);
            }
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Adr: {
            setAddress(value->bytes[0]);
            if(verbose) UARTprintf("New address: %d", getAddress());
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case MaxCur: {
            setMaxCurrent(value->f);
            if(verbose) {
                UARTprintf("New max current: ");
                UARTPrintFloat(getMaxCurrent(), false);
            }
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case EStop: {
            setEStop(value->bytes[0]);
            if(verbose) UARTprintf("New estop behavior: %x", value->bytes[0]);
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Tmp: {
            if(verbose) UARTprintf("Invalid set, user tried to set temperature\n");
            clearStatus(COMMAND_FAILURE);
            break;
        }

        default: {
            if(verbose) UARTprintf("Tried to set invaliad parameter, aborting\n");
            clearStatus(COMMAND_FAILURE);
            break;
        }
    }
    if(verbose) UARTprintf("\n");
}

/**
 * Send value of given parameter on UART.
 *
 * @param par - parameter to send
 * @param id - message identifier
 * @param verbose - whether or not to print out
 */
void getData(enum Parameter par, uint8_t id, bool verbose) {
    uint8_t NUM_BYTES_IN_FLOAT = 4;
    uint8_t numBytes;
    union Flyte value;
    switch(par) {
        case Pos: {
            if(verbose) {
                UARTprintf("Current pos: ");
                UARTPrintFloat(getAngle(), false);
            }
            value.f = getAngle();
            numBytes = NUM_BYTES_IN_FLOAT;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Vel: {
            if(verbose) {
                UARTprintf("Current vel: ");
                UARTPrintFloat(getVelocity(), false);
            }
            value.f = getVelocity();
            numBytes = NUM_BYTES_IN_FLOAT;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Cur: {
            if(verbose) {
                UARTprintf("Current current: ");
                UARTPrintFloat(getCurrent(), false);
            }
            value.f = getCurrent();
            numBytes = NUM_BYTES_IN_FLOAT;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Tmp: {
            if(verbose) {
                UARTprintf("Current temperature: ");
                UARTPrintFloat(getTemp(), false);
            }
            value.f = getTemp();
            numBytes = NUM_BYTES_IN_FLOAT;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case MaxCur: {
            if(verbose) {
                UARTprintf("Max Current Setting: ");
                UARTPrintFloat(getMaxCurrent(), false);
            }
            value.f = getMaxCurrent();
            numBytes = NUM_BYTES_IN_FLOAT;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        // <<<< SINGLE BYTE VALUES >>>>
        case EStop: {
            uint8_t temp = getEStop();
            if(verbose) UARTprintf("EStop behavior: %x", temp);
            value.bytes[0] = temp;
            numBytes = 1;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        case Status: {
            uint8_t temp = getStatus();
            if(verbose) UARTprintf("Status: %x", temp);
            value.bytes[0] = temp;
            numBytes = 1;
            setStatus(COMMAND_SUCCESS);
            break;
        }

        default: {
            UARTprintf("Asked for invalid parameter, aborting");
            clearStatus(COMMAND_FAILURE);
            break;
        }
    }
    if(verbose) UARTprintf("\n");

    uint8_t msg[5];
    if(!(getStatus() & ~COMMAND_FAILURE)) {
        msg[0] = getStatus();
        msg[1] = id;
        UARTSend(msg, 2, verbose);
    } else {
        msg[0] = id;
        int i;
        for(i = 0; i < numBytes; ++i)
            msg[i+1] = value.bytes[i];

        UARTSend(msg, numBytes + 1, verbose); // 1 for id
    }
}

// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// <<<<<<< MESSAGE HANDLING >>>>>>>
// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>

/**
 * Take actions on message as appropriate.
 *
 * If address does not match our own, bail out and send message on.
 *
 * @param buffer - pointer to the message
 * @param length - the length of the message
 * @param verbose - if true print to console for debugging
 * @return if response needed, valid id 0-255, else 65535 (max size of uint16_t)
 */
uint16_t handleUART(uint8_t* buffer, uint32_t length, bool verbose) {
    // get address
    uint8_t tempaddr = buffer[ADDR_IND];

    // return early on broadcast address
    if(tempaddr == BROADCAST_ADDR) return NO_RESPONSE;

    if(verbose) {
        int i;
        UARTprintf("****** Message ******\n");
        for (i = 0; i < length; ++i) UARTprintf("%x ", buffer[i]);
        UARTprintf("\n");
    }

    uint8_t crcin = recv[length-2];
    // check for corrupted message
    if(crc8(0, (uint8_t *)recv, length-2) != crcin) {
        UARTprintf("Corrupted message, panic!\n");
        return NO_RESPONSE;
    }

    if(verbose) UARTprintf("Addr - %x", tempaddr);

    uint8_t id = recv[ID_IND];
    if(verbose) UARTprintf(", ID - %x", id);

    // <<<< ADDRESS >>>>
    if(tempaddr == ADDRSET_ADDR) {
        setAddress(buffer[ADDRSET_IND]);
        if(verbose) UARTprintf("\nSet address to %x\n", getAddress());
        setStatus(COMMAND_SUCCESS);
        return (uint16_t) id;
    } else if(tempaddr != getAddress()) {
        if(verbose) UARTprintf("\nNot my address, abort\n");
        return NO_RESPONSE;
    }

    // <<<< COMMAND >>>>
    enum Command type = buffer[COM_IND] & CMD_MASK ? Set : Get;
    if(verbose) UARTprintf(", com - %s", getCommandName(type));

    enum Parameter par = buffer[COM_IND] & PAR_MASK;
    if(par > MAX_PAR_VAL) {
        if(verbose) UARTprintf("\nNo parameter specified, abort");
        clearStatus(COMMAND_FAILURE);
        return (uint16_t) id;
    }

    if(verbose) UARTprintf(", par - %s\n", getParameterName(par));

    // <<<< PARAMETER VALUE >>>>
    if(type == Set) {
        if (length < 5){
            if(verbose) UARTprintf("No value provided, abort\n");
            clearStatus(COMMAND_FAILURE);
            return (uint16_t) id;
        }

        // if cmd is Set then next entity is a value. This value is either a
        // single float (next four bytes of buffer) or a single byte
        union Flyte setval;
        int i;
        for (i = 0; i < length - 4; ++i)
            setval.bytes[i] = buffer[i+PAR_VAL_OFFSET];

        setData(par, &setval, verbose);
        return (uint16_t) id;
    } else {
        getData(par, id, verbose);
        return NO_RESPONSE;
    }
}

/**
 * Send message to UART connection.
 *
 * @param buffer - pointer to the message
 * @param length - the length of the message
 * @param verbose - whther or not to print out debugging statements
 */
void UARTSend(const uint8_t *buffer, uint32_t length, bool verbose) {
    uint8_t msg[8];
    msg[0] = BRAIN_ADDR;
    int i, len;
    len = 1;
    for (i = 0; i < length; ++i){
        msg[i+1] = buffer[i];
        len++;
    }

    // add CRC byte
    uint8_t crc = crc8(0, (const unsigned char*) &msg, len);
    msg[len++] = crc;
    msg[len++] = STOP_BYTE;

    UARTprintf("****** Sending ******\n");
    for(i = 0; i < len; ++i) UARTprintf("%x ", msg[i]);
    UARTprintf("\n*********************\n");

    bool space;
    for (i = 0; i < len; ++i) {
        // write the next character to the UART.
        // putchar returns false if the send FIFO is full
        space = ROM_UARTCharPutNonBlocking(UART7_BASE, msg[i]);
        // if send FIFO is full, wait until we can put the char in
        while (!space)
            space = ROM_UARTCharPutNonBlocking(UART7_BASE, msg[i]);
    }
}

// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// <<<<<<<<<<<< HANDLERS >>>>>>>>>>
// <<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>

/**
 * UART interrupt handler
 */
void UARTIntHandler(void) {
    // Get the interrrupt status.
    uint32_t ui32Status = ROM_UARTIntStatus(UART7_BASE, true);
    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART7_BASE, ui32Status);
    ROM_UARTIntDisable(UART7_BASE, UART_INT_RX | UART_INT_RT);

    // Loop while there are characters in the receive FIFO.
    // Read the next character from the UART and write it back to the UART.
    uint8_t character = ROM_UARTCharGetNonBlocking(UART7_BASE);
    // UARTprintf("Byte: %x\n", character);
    recv[recvIndex++] = character;

    if(character == STOP_BYTE) {
        HEARTBEAT_TIME = 0;
        uint8_t len = recvIndex;
        recvIndex = 0;

        if(len >= 3) {
            uint16_t id = handleUART(recv, len, /*verbose=*/true);
            if(id == NO_RESPONSE) {}
            else if(id < 256) {
                uint8_t msg[2];
                msg[0] = (uint8_t) id;
                msg[1] = getStatus();
                UARTSend(msg, 2, false);
            } else {
                // this should never happen
                UARTprintf("\n\n\n\n\nTHIS SHOULDN'T HAVE HAPPENED\n\n\n\n\n");
            }
        } else {
            UARTprintf("\n\n\n\n\nRECEIVED MESSAGE WITH LENGTH < 3\n\n\n\n\n");
        }
    }
    ROM_UARTIntEnable(UART7_BASE, UART_INT_RX | UART_INT_RT);
}
