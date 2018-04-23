/*
 * reflectance.c
 *
 *  Created on: Apr 1, 2018
 *      Author: allentang1996
 */

#include "reflectance.h"

void ReflectInit(void) {
#if defined(TARGET_IS_TM4C129_RA0) ||                                         \
    defined(TARGET_IS_TM4C129_RA1) ||                                         \
    defined(TARGET_IS_TM4C129_RA2)
    uint32_t ui32SysClock;
#endif

    //
    // Set the clocking to run at 20 MHz (200 MHz / 10) using the PLL.  When
    // using the ADC, you must either use the PLL or supply a 16 MHz clock
    // source.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //
#if defined(TARGET_IS_TM4C129_RA0) ||                                         \
    defined(TARGET_IS_TM4C129_RA1) ||                                         \
    defined(TARGET_IS_TM4C129_RA2)
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                       SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL |
                                       SYSCTL_CFG_VCO_480), 20000000);
#else
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);
#endif

    //
    // Display the setup on the console.
    //
    UARTprintf("Initializing optical encoder...\n");

    //
    // The ADC0 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // For this example ADC0 is used with AIN0 on port E7.
    // The actual port and pins used may be different on your part, consult
    // the data sheet for more information.  GPIO port E needs to be enabled
    // so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Select the analog ADC function for these pins.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0 );

    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE,0,0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE,0,1, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE,0,2, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE,0,3, ADC_CTL_CH3 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 0);

    //
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    //
    ADCIntClear(ADC0_BASE, 0);
}

uint32_t bin2int(char* binArray, uint8_t size) {
    uint32_t bin, i;
    bin = 0;
    for (i = 0; i < size; i++) {
        bin = bin << 1;
        bin += binArray[i];
    }
    return bin;
}

uint32_t getPosition(void) {
    //
    // Trigger the ADC conversion.
    //
    ADCProcessorTrigger(ADC0_BASE, 0);

    //
    // Wait for conversion to be completed.
    //
    while(!ADCIntStatus(ADC0_BASE, 0, false))
    {
    }

    //
    // Clear the ADC interrupt flag.
    //
    ADCIntClear(ADC0_BASE, 0);

    //
    // Read ADC Value.
    //
    ADCSequenceDataGet(ADC0_BASE, 0, pui32ADC0Value);

    //
    // Convert each sample to binary
    //
    int i;
    char binArray[NUM_CHANNELS];
    for (i = 0; i < NUM_CHANNELS; i++) {
        if (pui32ADC0Value[i] > THRESH) {
            binArray[i] = 1;
        } else {
            binArray[i] = 0;
        }
    }

    return bin2int(binArray, NUM_CHANNELS-1);

}
