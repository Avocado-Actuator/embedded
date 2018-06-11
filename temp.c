/**
 * Implementation of temperature reading.
 */

#include "temp.h"

float Temp, PrevTemp;

/**
 * Initialize temperature reading.
 *
 * @param g_ui32SysClock - system clock to sync with
 */
void TempInit(uint32_t ui32SysClock) {
    // The SSI0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    // Configure the pin muxing for SSI0 functions on port A2, A3, A4, and A5.
    // This step is not necessary if your part does not support pin muxing.
    GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    GPIOPinConfigure(GPIO_PB4_SSI1FSS);
    //GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);
    // Configure the GPIO settings for the SSI pins.  This function also gives
    // control of these pins to the SSI hardware.  Consult the data sheet to
    // see which functions are allocated per pin.
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    GPIOPinTypeSSI(GPIO_PORTE_BASE,  GPIO_PIN_5);

    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE,GPIO_PIN_4);
    GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_5);

    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_4);

    // Configure and enable the SSI port for SPI master mode.  Use SSI0,
    // system clock supply, idle clock level low and active low clock in
    // freescale SPI mode, master mode, 1MHz SSI frequency, and 16-bit data.
    // For SPI mode, you can set the polarity of the SSI clock when the SSI
    // unit is idle.  You can also configure what clock edge you want to
    // capture data on.  Please reference the datasheet for more information on
    // the different SPI modes.
    SSIConfigSetExpClk(SSI1_BASE, ui32SysClock, SSI_FRF_MOTO_MODE_1,
                           SSI_MODE_MASTER, 1000000, 16);
    // enable SSI0 module
    SSIEnable(SSI1_BASE);
    UARTprintf("Thermocouple initialized\n");
}

/**
 * Get current temperature.
 *
 * @return the current temperature
 */
float getTemp() { return Temp; }

/**
 * Set current temperature.
 *
 * @param newTemp - the new temperature value to update with
 */
void setTemp(float newTemp) { Temp = newTemp; }

/**
 * Update current temperature reading.
 */
void updateTemp() {
    uint32_t data;
    SSIDataPut(SSI1_BASE, 0);
    SSIDataGet(SSI1_BASE, &data);
    data >>= 2;

    float decimal = 0.25 * (data & 0x3);
    int intergral= data >> 2;
    PrevTemp = getTemp();
    setTemp(decimal + intergral);

    if(getTemp() > MAX_TEMP) {
        UARTprintf("Motor too hot! Freeze!");
        brake(getEStop());
    }
}
