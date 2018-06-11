/**
 * Implementation of position sensing through a magnetic encoder.
 */

#include "mag_encoder.h"

/**
 * Initialize reading of the magnetic encoder
 *
 * @param g_ui32SysClock - system clock to sync with
 */
void EncoderInit(uint32_t ui32SysClock) {
    // References
    // https://e2e.ti.com/support/microcontrollers/tiva_arm/f/908/t/432569
    // https://gist.github.com/madcowswe/e360649b1f8075c042b794ac550f600b
    volatile uint32_t pui32DataTx[NUM_SSI_DATA];
    uint32_t pui32DataRx[NUM_SSI_DATA];

    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    SSISysClock = ui32SysClock;

    // The SSI0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    // For this example SSI0 is used with PortA[5:2].  The actual port and pins
    // used may be different on your part, consult the data sheet for more
    // information.  GPIO port A needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Configure the pin muxing for SSI0 functions on port A2, A3, A4, and A5.
    // This step is not necessary if your part does not support pin muxing.
    // TODO: change this to select the port/pin you are using.
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);
    GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);

    // Configure the GPIO settings for the SSI pins.  This function also gives
    // control of these pins to the SSI hardware.  Consult the data sheet to
    // see which functions are allocated per pin.
    // The pins are assigned as follows:
    //      PA5 - SSI0 MISO
    //      PA4 - SSI0 MOSI
    //      PA3 - SSI0 Fss
    //      PA2 - SSI0 CLK
    // TODO: change this to select the port/pin you are using.
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                   GPIO_PIN_2);

    // Configure and enable the SSI port for SPI master mode.  Use SSI0,
    // system clock supply, idle clock level low and active low clock in
    // freescale SPI mode, master mode, 1MHz SSI frequency, and 16-bit data.
    // For SPI mode, you can set the polarity of the SSI clock when the SSI
    // unit is idle.  You can also configure what clock edge you want to
    // capture data on.  Please reference the datasheet for more information on
    // the different SPI modes.
    SSIConfigSetExpClk(SSI0_BASE, SSISysClock, SSI_FRF_MOTO_MODE_1,
                           SSI_MODE_MASTER, 1000000, 16);

    // Enable the SSI0 module.
    SSIEnable(SSI0_BASE);

    // Read any residual data from the SSI port.  This makes sure the receive
    // FIFOs are empty, so we don't read any unwanted junk.  This is done here
    // because the SPI SSI mode is full-duplex, which allows you to send and
    // receive at the same time.  The SSIDataGetNonBlocking function returns
    // "true" when data was returned, and "false" when no data was returned.
    // The "non-blocking" function checks if there is any data in the receive
    // FIFO and does not "hang" if there isn't.
    while(SSIDataGetNonBlocking(SSI0_BASE, &pui32DataRx[0])) { }
}

/**
 * Calculate even parity of bits in given value
 *
 * @param value - value to check even parity of
 */
static uint8_t calcEvenParity(uint32_t value) {
    uint8_t cnt = 0;
    uint8_t i;
    for (i = 0; i < 32; ++i) {
        if (value & 0x1) cnt++;
        value >>= 1;
    }
    return cnt & 0x1;
}

/**
 * Get reading from SSI?
 *
 * @return the value of the reading
 */
uint32_t sendNOP(void) {
    uint32_t data = 0x0000;
    SSIDataPut(SSI0_BASE, data);
    SSIDataGet(SSI0_BASE, &data);
    return data;
}

/**
 * Write given data to register at given address
 *
 * @param addr - the address of the register to write to
 * @param data - the data to write to the given address
 */
void writeRegister(uint32_t addr, uint32_t data) {
    uint32_t send;
    uint32_t receive[4];
    send = CMD_WRITE | addr;
    SSIDataPut(SSI0_BASE, send);
    SSIDataGet(SSI0_BASE, &receive[0]);
    send = data;
    send |= calcEvenParity(send) << 15;
    SSIDataPut(SSI0_BASE, send);
    SSIDataGet(SSI0_BASE, &receive[1]);
    UARTprintf("Old register contents: %x", receive[1]);
    receive[2] = sendNOP();
    UARTprintf("New register contents: %x", receive[2]);
}

/**
 * Read value of register at given address
 *
 * @param addr - the address of the register to read from
 */
uint32_t readRegister(uint32_t addr) {
    // send read command for given register, returns data from previous command
    uint32_t data = CMD_READ | addr;
    data |= calcEvenParity(data) << 15;
    SSIDataPut(SSI0_BASE, data);
    SSIDataGet(SSI0_BASE, &data);
    return data;
}

/**
 * Read magnetic encoder value
 *
 * @param angle - ?
 * @param mag - ?
 * @param agc - ?
 * @param alarmHi - ?
 * @param alarmLo - ?
 */
void readData(uint32_t* angle, uint32_t* mag, char* agc, char* alarmHi,
    char* alarmLo) {
    uint32_t recBuffer[5];
    uint32_t send;
    uint32_t value;
    // Send READ AGC command. throw away received data (it comes from previous
    // command)
    recBuffer[0] = readRegister(REG_AGC);

    // Send READ MAG command. Received data is the AGC value
    recBuffer[1] = readRegister(REG_MAG);
    *agc = recBuffer[1] & 0xff; // AGC value (0..255)
    *alarmHi = (recBuffer[1] >> 11) & 0x1;
    *alarmLo = (recBuffer[1] >> 10) & 0x1;

    // Send READ ANGLE command. Received data is the MAG value
    recBuffer[2] = readRegister(REG_DATA);
    *mag = recBuffer[2] & (16384 - 31 - 1);

    // Send NOP ANGLE command. Received data is the ANGLE value
    recBuffer[3] = readRegister(REG_DATA);
    value = recBuffer[3] & (16384 - 31 - 1); // Angle value (0.. 16384 steps)
    *angle = (value * 360) / 16384; // Angle value in degrees (0..360)

    // Check for errors
    uint8_t i;
    for(i = 1; i < 4; ++i) {
        if(recBuffer[i] & 0x4000) {
            /* error flag set */
            UARTprintf("Error flag set during %d command: ", i);
            send = CMD_READ | REG_CLRERR;
            send |= calcEvenParity(send) << 15;
            SSIDataPut(SSI0_BASE, send);
            SSIDataGet(SSI0_BASE, &recBuffer[4]);
            // Get error register contents
            recBuffer[4] = sendNOP();
            if((recBuffer[4] >> 2) & 0x1) { UARTprintf("Parity error\n"); }
            else if((recBuffer[4] >> 1) & 0x1) { UARTprintf("Invalid command\n"); }
            else if(recBuffer[4] & 0x1) { UARTprintf("Framing error\n"); }
            break;
        }
    }

    recBuffer[4] = sendNOP();
}

/**
 * Compute average magnetic encoder value over AVG_CYCLES computation cycles
 *
 * @param angle - pointer to current angle to write to (read outside function)
 * @param mag - ?
 * @param agc - ?
 */
void readAverageData(uint32_t* angle, uint32_t* mag, uint32_t* agc) {
    uint32_t intBuffer[3];
    char charBuffer[4];
    uint32_t count, alarmHi, alarmLo;
    *angle = 0;
    *mag = 0;
    *agc = 0;
    for(count = AVG_CYCLES; count; --count) {
        readData(&intBuffer[0], &intBuffer[1], &charBuffer[0], &charBuffer[1],
            &charBuffer[2]);
        *angle += intBuffer[0];
        *mag += intBuffer[1];
        *agc += charBuffer[0];
        alarmLo = charBuffer[1];
        alarmHi = charBuffer[2];
    }
    *angle = *angle/AVG_CYCLES;
    *mag = *mag/AVG_CYCLES;
    *agc = *agc/AVG_CYCLES;

    if (alarmLo) { UARTprintf("Warning: Low magnetic field\n"); }
    else if (alarmHi) { UARTprintf("Warning: High magnetic field\n"); }
}

/**
 * Set current angle as 0 position
 */
void zeroPosition(void) {
    UARTprintf("Zeroing position...\n");
    uint32_t recBuffer, angleValue;
    // write 0 to OTP zero position register
    writeRegister(REG_ZEROPOS_HI, 0x00);
    writeRegister(REG_ZEROPOS_LO, 0x00);

    // read current angle position
    recBuffer = readRegister(REG_DATA);
    recBuffer = sendNOP();
    angleValue = recBuffer & 0x3fff; // get rid of parity bit and error flag

    // write previous read angle position into OTP zero position register
    writeRegister(REG_ZEROPOS_HI, (angleValue >> 6)); // high byte
    writeRegister(REG_ZEROPOS_LO, (angleValue & 0x003f)); // 6 lower LSB
}
