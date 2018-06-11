/**
 * Implementation of position sensing through an optical sensor
 */

#include "reflectance.h"

/**
 * Initialize position reading through optical sensor.
 */
void ReflectInit(void) {
    // display setup on the console
    UARTprintf("Initializing optical encoder...\n");

    // the ADC0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Configure ADC clock to use the PLL (which is set at 480MHz) divided by 24
    // to get 20MHz and to sample at the full clock rate
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

/**
 * Convert binary number held in char array into int.
 *
 * @param binArray - binary number represented as a char array
 * @param size - the size of binArray
 * @return the converted integer
 */
uint32_t bin2int(char* binArray, uint8_t size) {
    uint32_t bin, i;
    bin = 0;
    for(i = 0; i < size; ++i) {
        bin <<= 1;
        bin += binArray[i];
    }
    return bin;
}

/**
 * Get which of the sections the encoder is currently detecting.
 *
 * @return the number of the section
 */
uint32_t getSection(void) {
    // trigger ADC conversion
    ADCProcessorTrigger(ADC0_BASE, 0);
    // wait for conversion
    while(!ADCIntStatus(ADC0_BASE, 0, false)) { }
    // clear ADC interrupt flag
    ADCIntClear(ADC0_BASE, 0);
    // read ADC value
    ADCSequenceDataGet(ADC0_BASE, 0, pui32ADC0Value);
    // convert each sample to binary
    int i;
    char greyArray[NUM_CHANNELS];
    for (i = 0; i < NUM_CHANNELS; ++i)
        greyArray[NUM_CHANNELS-1-i] = (pui32ADC0Value[i] > THRESH) ? 1 : 0;

    char binArray[NUM_CHANNELS];
    gray2bin(greyArray, binArray, NUM_CHANNELS);
    return bin2int(binArray, NUM_CHANNELS);
}

/**
 * Convert binary number held in char array into int.
 *
 * @param gray - the gray code samples
 * @param bin - binary number represented as a char array
 * @param size - the size of the bin array
 */
void gray2bin(char* gray, char* bin, uint8_t size) {
    // MSB of binary code is same as gray code
    bin[0] = gray[0];

    // compute remaining bits
    uint32_t i;
    for (i = 1; i < size; ++i) {
        // if current bit is 0, concatenate previous bit
        if (gray[i] == 0) { bin[i] = bin[i - 1]; }
        // concatenate invert of previous bit
        else { bin[i] = (~(bin[i - 1]) & 0x01); }
    }
}

/**
 * Calculate the final angle given both optical and magnetic data.
 *
 * @param angle - the angle within the section given by the magnetic encoder
 * @param section - which section of the optical encoder we're detecting
 * @return the final angle of our device
 */
float calcFinalAngle(uint32_t angle, uint32_t section) {
    uint32_t final_section;
    // check if mag encoder angle corresponds to correct half of section
    if ((section % 2 == 0) & (angle > 270)) {
        // reset section back 1
        final_section = section == 0 ? 7 : (section - 1) / 2;
    } else if ((section % 2 == 1) & (angle < 50)) {
        // set section forward by 1
        final_section = section == 15 ? 0 : (section + 1) / 2;
    } else { final_section = section/2; }

    return ((float) angle/360.0) * 45.0 + ((float) final_section * 45.0);
}
