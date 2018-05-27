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

uint32_t pui32DataTx[4];
uint32_t pui32DataRx[4];

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

void TimerInit(uint32_t g_ui32SysClock)
{
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
}

void PWMInit(){
    /***********PWM control***********/
    //Enable PWM output on PG0, which connect to INHA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //1/64 of system clock: 120Mhz/64
    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_1);//64

    //set PG0 as PWM pin
    GPIOPinConfigure(GPIO_PG0_M0PWM4);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    // From R2R:
    // Set the PWM period to 12500Hz.  To calculate the appropriate parameter
    // use the following equation: N = (1 / f) * SysClk.  Where N is the
    // function parameter, f is the desired frequency, and SysClk is the
    // system clock frequency.
    // In this case you get: (1 / 12500Hz) * 120MHz = 9600 cycles.  Note that
    // the maximum period you can set is 2^16 - 1.
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 9600);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
}

void MotorSPIInit(uint32_t ui32SysClock)
{
    // The SSI2 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2); // SSI2

    // Enable GPIO for SPI2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    // Configure the GPIO settings for the SSI pins.
    GPIOPinConfigure(GPIO_PD3_SSI2CLK); //CLK
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0); // MOSI
    GPIOPinConfigure(GPIO_PD0_SSI2XDAT1); // MISO
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3); // SCK/MOSI/MISO
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2 );//FSS
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);//set FSS to high

    // Configure and enable the SSI port for SPI master mode.
    //TODO: Test if the bit rate can be the highest 5M.
    SSIConfigSetExpClk(SSI2_BASE, ui32SysClock, SSI_FRF_MOTO_MODE_1,
                           SSI_MODE_MASTER, 1000000, 16); // 16 bits for motor, use 100kbps mode, use the system clock

    // Enable the SSI2 module.
    SSIEnable(SSI2_BASE);
}

void MotorSPISetting(){
    while(SSIDataGetNonBlocking(SSI2_BASE, &pui32DataRx[0])){
    }

    pui32DataTx[0] = 0b1001000000000000; // read register 2h
    pui32DataTx[1] = 0b0001000001000000; // set register 2h, bit 6 and 5 to 10, option 3, 1x PWM mode, bit 4 to 0, synchronous
    pui32DataTx[2]=  0b1001000000000000; // read register 2h
    pui32DataTx[3]=  0b1001000000000000; // read register 2h one more time

    GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,0);
    SysCtlDelay(1);
    int i;
    for(i = 0; i < 4; i++){ // only reading and writing 3 times
        GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,0); // Pull FSS low
        SysCtlDelay(1);
        SSIDataPut(SSI2_BASE, pui32DataTx[i]); // Send data
        SysCtlDelay(1000); // wait (at least 50ns)
        GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,GPIO_PIN_2); // Set FSS high
        SSIDataGet(SSI2_BASE, &pui32DataRx[i]); // Get the data
    }
    while(SSIBusy(SSI2_BASE)){
    }

    GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,GPIO_PIN_2); // Make sure the pin is high

   SysCtlDelay(1000);
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

    UARTprintf("initializing motor driver...\n");

    TimerInit(g_ui32SysClock);

    PWMInit();

    //Motor pins configuration
    //Set PQ1 Always high for enable
    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,GPIO_PIN_1);

    //Set PP0 as INLC, spins counterclockwise if PP0 is high
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_0,GPIO_PIN_0);

	//Set PP1 as INHC, brake if PP1 is low
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);


    MotorSPIInit(g_ui32SysClock);
    MotorSPISetting();
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
    UARTprintf("final angle: %d\n\n", (int)getAngle());
    return;
}

float getVelocity() { return VELO; }
void setVelocity(float newVelocity) { VELO = newVelocity; }
float getTargetVelocity() { return TARGET_VELO; }
void setTargetVelocity(float newVelocity) { TARGET_VELO = newVelocity; }

// unit: degree per second
void UpdateVelocity() {
    float diff;
    diff= getAngle()- PrevAngle;
    //use acute angle
    //the motor move from  350 to 10, the diff should be 20 instead of -340, etc
    diff=diff<-180?diff+360:diff;
    diff=diff>180?diff-360:diff;
	setVelocity(diff/ 0.00002); // assumes measuring velocity every 20ns
}

void PWMoutput(int dut){
    if(dut<0){
        dut=-dut;
        //change the output direction
    }
    //TODO: test what if duty is 0
    if(dut==0){
        dut=1;
    }
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 9600*dut/100);
}

//get duty width
uint32_t getPWM(){
    return PWMPulseWidthGet(PWM0_BASE, PWM_GEN_2);
}


void brake(){
    PWMoutput(0);
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
    error=error<-180?error+360:error;
    error=error>180?error-360:error;
	if ( error<=5 && error>=-5){//if reach the target position, trigger break
	    //maybe you want to hold it instead of brake like
	    //VelocityControl(0);
		brake();
		return;
	}
    //error*=direction;
    //error=error>0?error:error+360;
    angleErrorInt+=error;
    if(angleErrorInt>=300){
        angleErrorInt=300;
    }
    float outputVelo=KP_angle*error + KI_angle*angleErrorInt;// + KD_angle*angleErrorDiff;//P*e(k)+I*sigma(e)+D*(e(k)-e(k-1))
    //lastAngleError=currentAngleError;
    //regulator

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

   PWMoutput((int)duty);
}


