/*
 * isense.c
 *
 *  Created on: Apr 20, 2018
 *      Author: Ryan
 */

#include "isense.h"
#include "motor.h"

float Current, TargetCurrent, PrevCurrent, MaxCurrent;

uint32_t offset[3]={0,0,0};

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
    ADCSequenceStepConfigure(ADC0_BASE,2,0, ADC_CTL_CH6);
    ADCSequenceStepConfigure(ADC0_BASE,2,1, ADC_CTL_CH5);
    ADCSequenceStepConfigure(ADC0_BASE,2,2, ADC_CTL_CH4 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 2);
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    ADCIntClear(ADC0_BASE, 2);

    PWMoutput(0);
    SysCtlDelay(12000000);
    // Determine offset for each CSA
    int j;
    for (j=0;j<OFFSET_SAMPLES;j++){
            ADCProcessorTrigger(ADC0_BASE, 2); // Trigger the ADC conversion.
            while(!ADCIntStatus(ADC0_BASE, 2, false)){} // Wait for conversion to be completed.
            ADCIntClear(ADC0_BASE, 2); // Clear the ADC interrupt flag.
            ADCSequenceDataGet(ADC0_BASE, 2, isensereadings); // Read ADC Value.
            int i;
            for (i = 0; i < CURRENT_CHANNELS; i++ ){
//                UARTprintf("Reading %d: %d\n", i, (int)isensereadings[i]);
                offset[i]+=isensereadings[i];
            }
        }
    for(j=0;j<3;j++){
        offset[j]=2048-offset[j]/OFFSET_SAMPLES;
        UARTprintf("offset %d:%d\n",j,offset[j]);
    }

    UARTprintf("Current sensors initialized\n");
}

float getCurrent() { return Current; }
void setCurrent(float newCurrent) { Current = newCurrent; }
float getTargetCurrent() { return TargetCurrent; }
void setTargetCurrent(float newCurrent) { TargetCurrent = newCurrent; }
float getMaxCurrent() { return MaxCurrent; }
void setMaxCurrent(float newMaxCurrent) { MaxCurrent = newMaxCurrent; }

void updateCurrent() {
    float volt,sum=0;
    int j;
    for (j=0;j<CURRENT_SAMPLES;j++){
        ADCProcessorTrigger(ADC0_BASE, 2); // Trigger the ADC conversion.
        while(!ADCIntStatus(ADC0_BASE, 2, false)){} // Wait for conversion to be completed.
        ADCIntClear(ADC0_BASE, 2); // Clear the ADC interrupt flag.
        ADCSequenceDataGet(ADC0_BASE, 2, isensereadings); // Read ADC Value.
        uint32_t h1 = GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_5); // Read Hall Sensors
        uint32_t h2 = GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_3);
        uint32_t h3 = GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_2);

        int i;
        for (i = 0; i < CURRENT_CHANNELS; i++ ){
            //UARTprintf("Raw ADC %d:%d\n",i,isensereadings[i]);
            volt = (float)(isensereadings[i]+offset[i]) / 4095.0 * 3.3; // turns analog value into voltage
//			UARTprintf("Voltage %d:%d\n",i,(int)(volt*1000));
            // i = (Vref / 2 - Vmeasured) / (gain * Rsense)
            isense[i]=(3.3/2 - volt) / (CSA_GAIN * 0.031);
        }

//		UARTprintf("H1:%d\n",(int)h1);
//		UARTprintf("H2:%d\n",(int)h2);
//		UARTprintf("H3:%d\n",(int)h3);
		if(direction==1){
            if(h1>0 && h2==0){
                sum+=isense[0];
            }
            else if(h2>0 && h3==0){
                sum+=isense[1];
            }
            else if(h3>0 && h1==0){
                sum+=isense[2];
            }
		}
		else{
		    if(h1==0 && h2>0){
                sum+=isense[0];
            }
            else if(h2==0 && h3>0){
                sum+=isense[1];
            }
            else if(h3==0 && h1>0){
                sum+=isense[2];
            }
		}
    }

//	UARTprintf("Current :%d\n",(int)(sum*1000));

    PrevCurrent = getCurrent();
    setCurrent(1000*sum/CURRENT_SAMPLES);//unit mA
    if(getCurrent()>MAX_CURRENT){
        brake();
    }
    return;
}
