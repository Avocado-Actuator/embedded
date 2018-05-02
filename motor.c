/*
 * motor.c
 */

#include "motor.h"

// velocity
float   Velocity,
        TargetVelocity,
        currentVeloError,
        lastVeloError,
        prevVeloError,
        frequency, //2500-9000
        duty;

// angle
float   Angle,
        TargetAngle,
        PrevAngle,
        currentAngleError,
        lastAngleError,
        angleErrorInt,
        angleErrorDiff;

void Timer0IntHandler(void)
{
    // Clear the timer interrupt.
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    // Update the interrupt status.
    Time++;
    if (Time%20000==0){
            time_flag_200ms=1;
        }
    if (Time%100000==0){
        time_flag_1000ms=1;
    }
    if (Time%2==0){//actually 20ns
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
    frequency = 5000; //2500-9000
    duty=0;
    KP_velocity=50;
    KI_velocity=90;
    KD_velocity=0;
    flag=0;

    KP_angle=0.05;
    KI_angle=0.001;
    KD_angle=0;

    //setAngle(0);
    setTargetAngle(200);
    currentAngleError=0;
    /************** Initialization for timer (1ms)  *****************/
    //Enable the timer peripherals
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    // Configure 32-bit periodic timers.
    //1ms timer
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock/100000);//was 1000, trigger every 1ms, 1000Hz
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

    //Set PP0 and PP1 high as DIR and nBreak, respectively
    //counterclockwise if PP0 is high
    //break if PP1 is low
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_0,GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);
}

//Incremental PID control of velocity
void VelocityControl()
{
    currentVeloError = TargetVelocity - getVelocity();

    UARTprintf("PWM output:%d.  ",frequency);
    UARTprintf("Target(count per 0.2sec):%d.  ", TargetVelocity);
    UARTprintf("speed(count per 0.2sec):%d.  ", getVelocity());
    UARTprintf("KI:%d. ",currentVeloError);
    UARTprintf("KP:%d. ",currentVeloError-lastVeloError);
    UARTprintf("KD:%d. \n",currentVeloError+prevVeloError-2*lastVeloError);

    duty+=KP_velocity*(currentVeloError-lastVeloError) + KI_velocity*currentVeloError + KD_velocity*(currentVeloError+prevVeloError-2*lastVeloError);//P*(e(k)-e(k-1))+I*e(k)+D*((e(k)-e(k-1))-(e(k-1)-e(k-2)))
    prevVeloError=lastVeloError;
    lastVeloError=currentVeloError;

    //regulator
    if (duty<=0)
        duty=0;
    if (duty>=90)
        duty=90;

    //PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency*duty/100);
}



//PD control of position
void PositionControl()
{
    int width=0;
    int angle= (int) getAngle();
    int target_angle=(int)getTargetAngle();
    currentAngleError = target_angle - angle;
    if(currentAngleError<0){
        currentAngleError*=-1;
    }
    angleErrorInt+=currentAngleError;
    if(angleErrorInt>=300){
        angleErrorInt=300;
    }
    //angleErrorDiff=currentAngleError-lastAngleError;
    width=(int)(KP_angle*currentAngleError + KI_angle*angleErrorInt);// + KD_angle*angleErrorDiff;//P*e(k)+I*sigma(e)+D*(e(k)-e(k-1))
    //lastAngleError=currentAngleError;
    //regulator
    if (width<=0)
        width=0;
    if (width>=18)
        width=18;


    //if reach the target position, trigger break
    if(currentAngleError<=10){
        UARTprintf("\nFinal Angle: %d",angle);
        UARTprintf("\nTarget Angle: %d",target_angle);
        //UARTPrintFloat(angle, false);
        UARTprintf("\nbraking");
        brake();
    }

    else{


        //frequency=5000;

        //PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/frequency);
        PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, width);
    }

}


//constant speed
void PositionControlCS()
{
    currentAngleError= TargetAngle - getAngle();
    if(currentAngleError<0){
        setAngle(getAngle() + 360);
    }
    if(currentAngleError<=30){
        brake();
    }
    else{
        GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);
        testSpin(5000,8);
    }
}

float getAngle() { return Angle; }
void setAngle(float newAngle) { Angle = newAngle; }
float getTargetAngle() { return TargetAngle; }
void setTargetAngle(float newAngle) { TargetAngle = newAngle; }

void updateAngle() {
    uint32_t angle, mag, agc, section;
    //Average data over number of cycles
    readAverageData(&angle, &mag, &agc);
    section = getSection();
    PrevAngle = getAngle();
    // Calculate final angle position in degrees
    setAngle(calcFinalAngle(angle, section));
    return;
}

float getVelocity() { return Velocity; }
void setVelocity(float newVelocity) { Velocity = newVelocity; }
float getTargetVelocity() { return TargetVelocity; }
void setTargetVelocity(float newVelocity) { TargetVelocity = newVelocity; }

// unit: degree per second
void UpdateVelocity() {
    if (getAngle() - PrevAngle>=0){
        setVelocity((getAngle()- PrevAngle)/ 0.002); // assumes measuring velocity every 2ms
    } else {
        setVelocity((getAngle() + 360 - PrevAngle) / 0.002);
    }
}

void testSpin(uint32_t freq, uint32_t dut){
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/freq);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1875000/freq*dut/100);
}

void brake(){
    flag=1;
    testSpin(5000,0);

//    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,0);
}

void disableDriver(){
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,0);
}
