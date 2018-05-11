/*
 * motor.c
 */

#include "motor.h"

float   frequency,
        duty;
int     direction=-1;//clockwise if direction is 1, counterclockwise if direcion=-1

// angle
float   ANGLE,
        TARGET_ANGLE,
        PrevAngle,
        angleErrorInt;

// velocity
float   VELO,
        TARGET_VELO,
        lastVeloError,
        prevVeloError,
        outputCurrent;

// current
float   CUR,
        TARGET_CUR,
        lastCurError,
        prevCurError;

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
        time_flag_20ns=1;
    }
}

void MotorInit(uint32_t g_ui32SysClock)
{
    // variable inits
    Time=0;
    time_flag_200ms = 0;
    time_flag_1000ms = 0;
    time_flag_20ns = 0;
    frequency = 5000; //2500-9000
    duty=0;

    KP_velocity=50;
    KI_velocity=90;
    KD_velocity=0;

    KP_angle=0.05;
    KI_angle=0.001;
    KD_angle=0;

    KP_current=0;
    KI_current=0;
    KD_current=0;

    /************** Initialization for timer (1ms)  *****************/
    //Enable the timer peripherals
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    // Configure 32-bit periodic timers.
    //1ms timer
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock/100000);//was 1000, now trigger every 10ns, 100000Hz
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
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    UARTprintf("initializing motor driver pins\n");
    //Set PQ1 Always high for enable
    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,GPIO_PIN_1);

    //Set PP0 as DIR, spins counterclockwise if PP0 is high
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_0,GPIO_PIN_0);

	//Set PP1 as nBreak, brake if PP1 is low
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);
}

float getAngle() { return ANGLE; }
void setAngle(float newAngle) { ANGLE = newAngle; }
float getTargetAngle() { return TARGET_ANGLE; }
void setTargetAngle(float newAngle) { TARGET_ANGLE = newAngle; }

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

float getVelocity() { return VELO; }
void setVelocity(float newVelocity) { VELO = newVelocity; }
float getTargetVelocity() { return TARGET_VELO; }
void setTargetVelocity(float newVelocity) { TARGET_VELO = newVelocity; }

// unit: degree per second
void UpdateVelocity() {
    float diff;
	if(direction==1){//clockwise
	    diff= getAngle()- PrevAngle;
	}
	else{
	    diff= PrevAngle - getAngle();
	}
    diff=diff>0?diff:diff+360;
	setVelocity(diff/ 0.00002); // assumes measuring velocity every 20ns
}

void PWMoutput(uint32_t freq, uint32_t dut){
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 1875000/freq);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1875000/freq*dut/100);
}

//get duty percentage
uint32_t getPWM(){
    return PWMPulseWidthGet(PWM0_BASE, PWM_GEN_2)*100*frequency/1875000;
}


void brake(){
    PWMoutput(5000,0);
	UARTprintf("\nbraking");
    //untested
    //GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,0);
    //PWMoutput(5000,0);
    //GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);
}

void disableDriver(){
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,0);
}

void enableDriver(){
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);
}

//PD control of position
void PositionControl(float target)
{
    float error = target - getAngle();
	
	if ( error<=5 && error>=-5){//if reach the target position, trigger break
	    //maybe you want to hold it instead of brake like
	    //VelocityControl(0);
		brake();
		return;
	}
    error*=direction;
    error=error>0?error:error+360;
    angleErrorInt+=error;
    if(angleErrorInt>=300){
        angleErrorInt=300;
    }
    float outputVelo=KP_angle*error + KI_angle*angleErrorInt;// + KD_angle*angleErrorDiff;//P*e(k)+I*sigma(e)+D*(e(k)-e(k-1))
    //lastAngleError=currentAngleError;
    //regulator
    if (outputVelo<0)
        outputVelo=0;

	VelocityControl(outputVelo);
}

//Incremental PID control of velocity
void VelocityControl(float target)
{
    float error = target - getVelocity();
    outputCurrent+=KP_velocity*(error-lastVeloError) + KI_velocity*error + KD_velocity*(error+prevVeloError-2*lastVeloError); //P*(e(k)-e(k-1))+I*e(k)+D*((e(k)-e(k-1))-(e(k-1)-e(k-2)))
    prevVeloError=lastVeloError;
    lastVeloError=error;

    //regulator
    if (outputCurrent<0)
        outputCurrent=0;

    CurrentControl(outputCurrent);
}

//PI control
void CurrentControl(float target)
{
    float error=target-getCurrent();//current sample rate should be consistent with the position
	duty+=KP_current*(error-lastCurError) + KI_current*error + KD_current*(error+prevCurError-2*lastCurError);//P*(e(k)-e(k-1))+I*e(k)+D*((e(k)-e(k-1))-(e(k-1)-e(k-2)))
	prevCurError=lastCurError;
	lastCurError=error;
    //regulator
	if(duty<0){
		duty=0;
	}

   PWMoutput(5000,(int)duty);
}


