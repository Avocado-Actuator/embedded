/*
 * reflectance.c
 *
 *  Created on: Apr 1, 2018
 *      Author: allentang1996
 */

#include "reflectance.h"

void ReflectInit(void) {
    // Display the setup on the console.
    UARTprintf("Initializing optical encoder...\n");

    // The ADC0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Configure ADC clock to use the PLL (which is set at 480MHz) divided by 24 to get 20MHz
    // and to sample at the full clock rate
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 24);

    // For this example ADC0 is used with AIN0 on port E7.
    // The actual port and pins used may be different on your part, consult
    // the data sheet for more information.  GPIO port E needs to be enabled
    // so these pins can be used.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    // Select the analog ADC function for these pins.
    // Consult the data sheet to see which functions are allocated per pin.
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0 );

    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE,0,0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE,0,1, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE,0,2, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE,0,3, ADC_CTL_CH3 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 0);

    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
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

uint32_t getSection(void) {
    // Trigger the ADC conversion.
    ADCProcessorTrigger(ADC0_BASE, 0);
    // Wait for conversion to be completed.
    while(!ADCIntStatus(ADC0_BASE, 0, false)){}
    // Clear the ADC interrupt flag.
    ADCIntClear(ADC0_BASE, 0);
    // Read ADC Value.
    ADCSequenceDataGet(ADC0_BASE, 0, pui32ADC0Value);
    // Convert each sample to binary
    int i;
    char greyArray[NUM_CHANNELS];
    for (i = 0; i < NUM_CHANNELS; i++) {
//        UARTprintf("%d: %d\n",i,pui32ADC0Value[i]);
        if (pui32ADC0Value[i] > THRESH) {
            greyArray[NUM_CHANNELS-1-i] = 1;
        } else {
            greyArray[NUM_CHANNELS-1-i] = 0;
        }
    }

    char binArray[NUM_CHANNELS];
    gray2bin(greyArray, binArray, NUM_CHANNELS);
    return bin2int(binArray, NUM_CHANNELS);
}

void gray2bin(char* gray, char* bin, uint8_t size)
{

    // MSB of binary code is same as gray code
    bin[0] = gray[0];

    // Compute remaining bits
    uint32_t i;
    for (i = 1; i < size; i++) {
        // If current bit is 0, concatenate
        // previous bit
        if (gray[i] == 0) {
            bin[i] = bin[i - 1];
        }
        // Else, concatenate invert of
        // previous bit
        else {
            bin[i] = (~(bin[i - 1]) & 0x01);
        }
    }
}

float calcFinalAngle(uint32_t angle, uint32_t section) {
    uint32_t final_section;
    // Check if the mag encoder angle corresponds to correct half of section
    if ((section % 2 == 0) & (angle > 270)) {
        // Reset section back 1
        if (section == 0) {
            final_section = 7;
        } else {
            final_section = (section - 1)/2;
        }
    } else if ((section % 2 == 1) & (angle < 90)) {
        // Set section forward by 1
        if (section == 15) {
            final_section = 0;
        } else {
            final_section = (section + 1)/2;
        }
    } else {
        final_section = section/2;
    }
//    UARTprintf("section:%d\n",(int)section);
//    UARTprintf("Angle:%d\n",(int)angle);
//    UARTprintf("final_section:%d\n",(int)final_section);
    return ((float) angle/360.0)*45.0 + (float) final_section*45.0;
}
