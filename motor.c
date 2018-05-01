/*
 * motor.c
 */

#include "motor.h"

void Timer0IntHandler(void)
{
    // Clear the timer interrupt.
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    // Update the interrupt status.
    Time++;
    if (Time%200==0){
            time_flag_200ms=1;
        }
    if (Time%1000==0){
        time_flag_1000ms=1;
    }
    if (Time%2==0){
        time_flag_2ms=1;
    }
}

void MotorInit(uint32_t g_ui32SysClock)
{
    // variable inits
    Time=0;
    time_flag_200ms = 0;
    time_flag_1000ms = 0;
    time_flag_2ms = 0;
    frequency = 0; //2500-9000
    duty=25;
    KP_velocity=50;
    KI_velocity=90;
    KD_velocity=0;

    /************** Initialization for timer (1ms)  *****************/
    //Enable the timer peripherals
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    // Configure 32-bit periodic timers.
    //1ms timer
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock/1000);
    // Setup the interrupts for the timer timeouts.
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    // Enable the timers.
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

    /***********PWM control***********/
    //Enable PWM output on PG0, which connect to INHA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //1/64 of system clock: 120Mhz/64
    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);

    //set PG0 as PWM pin
    GPIOPinConfigure(GPIO_PG0_M0PWM4);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 320);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 20);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    UARTprintf("initializing motor driver pins\n");
    //Set PQ1 Always high for enable
    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,GPIO_PIN_1);

    //Set PP0 and PP1 high as nBRAKE and DIR, respectively
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_0,GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);
}

//Incremental PID control of velocity
void VelocityControl()
{
    currentVeloError=TargetVelocity-CurrentVelocity;

    UARTprintf("PWM output:%d.  ",frequency);
    UARTprintf("Target(count per 0.2sec):%d.  ",TargetVelocity);
    UARTprintf("speed(count per 0.2sec):%d.  ",CurrentVelocity);
    UARTprintf("KI:%d. ",currentVeloError);
    UARTprintf("KP:%d. ",currentVeloError-lastVeloError);
    UARTprintf("KD:%d. \n",currentVeloError+prevVeloError-2*lastVeloError);

    frequency+=KP_velocity*(currentVeloError-lastVeloError) + KI_velocity*currentVeloError + KD_velocity*(currentVeloError+prevVeloError-2*lastVeloError);//P*(e(k)-e(k-1))+I*e(k)+D*((e(k)-e(k-1))-(e(k-1)-e(k-2)))
    prevVeloError=lastVeloError;
    lastVeloError=currentVeloError;

    //regulator
    if (frequency<=0)
        frequency=0;
    if (frequency>=10000)
        frequency=10000;

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency*duty/100);
}


uint32_t TargetAngle;
uint32_t CurrentAngle;
uint32_t currentAngleError;
uint32_t lastAngleError;
uint32_t angleErrorInt=0;
uint32_t angleErrorDiff;

//Incremental PID control of velocity
void PositionControl()
{
    currentAngleError=TargetAngle-CurrentAngle;
    angleErrorInt+=currentAngleError;
    angleErrorDiff=currentAngleError-lastAngleError;
    frequency+=KP_angle*currentAngleError + KI_angle*angleErrorInt + KD_angle*angleErrorDiff;//P*e(k)+I*sigma(e)+D*(e(k)-e(k-1))
    lastAngleError=currentAngleError;

    //regulator
    if (frequency<=0)
        frequency=0;
    if (frequency>=10000)
        frequency=10000;

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency*duty/100);
}

void GetAngle(){
    uint32_t angle, mag, agc, section;
    //Average data over number of cycles
    readAverageData(&angle, &mag, &agc);
    section = getSection();
    PrevAngle = CurrentAngle;
    // Calculate final angle position in degrees
    CurrentAngle = calcFinalAngle(angle, section);
    return;
}

void GetVelocity(){
    CurrentVelocity = (CurrentAngle - PrevAngle)/ 0.002; // assumes measuring velocity every 1ms
    return;
}

void testSpin(uint32_t freq, uint32_t dut){
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/freq);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1875000/freq*dut/100);
}
