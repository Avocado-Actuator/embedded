/*
 * isense.c
 *
 *  Created on: Apr 20, 2018
 *      Author: Ryan
 */

#include "isense.h"

void CurrentSenseInit(void) {
    // Display the setup on the console.
    UARTprintf("Initializing Current Sensing...\n");
    // The ADC0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // Using pins on port D
    // Select the analog ADC function for these pins.
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5);
    // Assign a sequence sampler, trigger source, and interrupt priority
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
    // Assign steps in the reading process. Three current sensors means three steps
    ADCSequenceStepConfigure(ADC0_BASE,2,0, ADC_CTL_CH4);
    ADCSequenceStepConfigure(ADC0_BASE,2,1, ADC_CTL_CH5);
    ADCSequenceStepConfigure(ADC0_BASE,2,2, ADC_CTL_CH6 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 2);
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    ADCIntClear(ADC0_BASE, 2);
}

void getCurrent(uint32_t* currents) {
    ADCProcessorTrigger(ADC0_BASE, 2); // Trigger the ADC conversion.
    while(!ADCIntStatus(ADC0_BASE, 2, false)){} // Wait for conversion to be completed.
    ADCIntClear(ADC0_BASE, 2); // Clear the ADC interrupt flag.
    ADCSequenceDataGet(ADC0_BASE, 2, isensereadings); // Read ADC Value.
    int i;
    for (i = 0; i <= CURRENT_CHANNELS; i++ ){
        currents[i] = isensereadings[i];
    }
    return;
}
