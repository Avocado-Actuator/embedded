/**
 * Implementation of current sensing.
 */

#include "isense.h"

float Current, TargetCurrent, PrevCurrent, MaxCurrent;

uint32_t offset[3]={0,0,0};

/**
 * Initialize current reading.
 */
void CurrentSenseInit(void) {
    // display the setup on the console
    // ADC0 peripheral must be enabled for use
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // using port D pins
    // configure ADC clock to use PLL (set at 480MHz) divided by 24 to get 20MHz
    // and to sample at the full clock rate
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 24);
    // select the analog ADC function for these pins
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5);
    // assign a sequence sampler, trigger source, and interrupt priority
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
    // assign steps in reading process, three current sensors means three steps
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH6);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, ADC_CTL_CH5);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 2, ADC_CTL_CH4 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 2);
    // clear interrupt status flag
    // ensures interrupt flag cleared before sampling
    ADCIntClear(ADC0_BASE, 2);

    PWMoutput(0);
    SysCtlDelay(12000000);

    // determine offset for each CSA
    int j;
    for(j = 0; j < OFFSET_SAMPLES; ++j) {
        ADCProcessorTrigger(ADC0_BASE, 2); // trigger ADC conversion
        while(!ADCIntStatus(ADC0_BASE, 2, false)) { } // wait for conversion

        ADCIntClear(ADC0_BASE, 2); // clear ADC interrupt flag
        ADCSequenceDataGet(ADC0_BASE, 2, isensereadings); // read ADC value
        int i;
        for(i = 0; i < CURRENT_CHANNELS; ++i)
            offset[i] += isensereadings[i];
    }
    for(j = 0; j < 3; ++j) {
        offset[j]= 2048 - offset[j]/OFFSET_SAMPLES;
        UARTprintf("offset %d:%d\n", j, offset[j]);
    }

    UARTprintf("Current sensors initialized\n");
}

/**
 * Update our most recent current reading
 *
 * Read the three Hall sensors and compute a current reading.
 */
void updateCurrent(void) {
    float volt, sum = 0;
    int j;
    for(j = 0; j < CURRENT_SAMPLES; ++j) {
        ADCProcessorTrigger(ADC0_BASE, 2); // trigger ADC conversion
        while(!ADCIntStatus(ADC0_BASE, 2, false)) { } // wait for conversion

        ADCIntClear(ADC0_BASE, 2); // clear ADC interrupt flag
        ADCSequenceDataGet(ADC0_BASE, 2, isensereadings); // read ADC value
        // read Hall sensors
        uint32_t h1 = GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_5);
        uint32_t h2 = GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_3);
        uint32_t h3 = GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_2);

        int i;
        for(i = 0; i < CURRENT_CHANNELS; ++i) {
            // turn analog value into voltage
            volt = (float) (isensereadings[i] + offset[i]) / 4095.0 * 3.3;
            isense[i]=(3.3/2 - volt) / (CSA_GAIN * 0.055);
        }

		if(direction == 1) {
            if(h1 > 0 && h2 == 0) { sum += isense[0]; }
            else if(h2 > 0 && h3 == 0) { sum += isense[1]; }
            else if(h3 > 0 && h1 == 0) { sum += isense[2]; }
		} else {
		    if(h1 == 0 && h2 > 0){ sum += isense[0]; }
            else if(h2 == 0 && h3 > 0) { sum += isense[1]; }
            else if(h3 == 0 && h1 > 0) { sum += isense[2]; }
		}
    }

    PrevCurrent = getCurrent();
    // unit mA
    setCurrent(1000 * sum/CURRENT_SAMPLES);

    if(getCurrent() > MAX_CURRENT) {
        UARTprintf("Drawing too much current! Freeze!");
        brake(getEStop());
    }
}

/**
 * Get the current current reading
 *
 * @return the current current in mA
 */
float getCurrent() { return Current; }

/**
 * Set the current value
 *
 * @param newCurrent - the new current value (in mA) to update with
 */
void setCurrent(float newCurrent) { Current = newCurrent; }

/**
 * Get the current target current
 *
 * @return the current target current in mA
 */
float getTargetCurrent() { return TargetCurrent; }

/**
 * Set the current target current
 *
 * @param the new target current in mA
 */
void setTargetCurrent(float newCurrent) { TargetCurrent = newCurrent; }

/**
 * Get the current maximum allowable current
 *
 * @return the current maximum allowable current in mA
 */
float getMaxCurrent() { return MaxCurrent; }

/**
 * Sets a new maximum current
 *
 * @param newMaxCurrent - the new max current value in mA
 */
void setMaxCurrent(float newMaxCurrent) { MaxCurrent = newMaxCurrent; }
