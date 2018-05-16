/*
 * isense.c
 *
 *  Created on: Apr 20, 2018
 *      Author: Ryan
 */

#include "isense.h"

float Current, TargetCurrent, PrevCurrent, MaxCurrent;

void CurrentSenseInit(void) {
    // Display the setup on the console.
    // The ADC0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // Using pins on port D
    // Configure ADC clock to use the PLL (which is set at 480MHz) divided by 24 to get 20MHz
    // and to sample at the full clock rate
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 24);
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
    UARTprintf("Current sensors initialized\n");
}

float getCurrent() { return Current; }
void setCurrent(float newCurrent) { Current = newCurrent; }
float getTargetCurrent() { return TargetCurrent; }
void setTargetCurrent(float newCurrent) { TargetCurrent = newCurrent; }
float getMaxCurrent() { return MaxCurrent; }
void setMaxCurrent(float newMaxCurrent) { MaxCurrent = newMaxCurrent; }

void updateCurrent() {
    ADCProcessorTrigger(ADC0_BASE, 2); // Trigger the ADC conversion.
    while(!ADCIntStatus(ADC0_BASE, 2, false)){} // Wait for conversion to be completed.
    ADCIntClear(ADC0_BASE, 2); // Clear the ADC interrupt flag.
    ADCSequenceDataGet(ADC0_BASE, 2, isensereadings); // Read ADC Value.
    int i;
    float volt, sum = 0;
    for (i = 0; i <= CURRENT_CHANNELS; i++ ){
        volt = isensereadings[i] / 4095 * 3.3; // turns analog value into voltage
        // i = (Vref / 2 - Vmeasured) / (gain * Rsense)
        sum += (3.3/2 - volt) / (10 * 0.014);
    }
    PrevCurrent = getCurrent();
    setCurrent(sum);
    return;
}
