/**
 * Implementation of motor control.
 */

#include "motor.h"

/* LSB of STATUS byte is estop behaviour: 1 is hold position until safety
threshold reached, 0 is kill motor power. Next bit up is success or failure of
most recent command. 1 success, 0 failure. Next up is whether output performance
is being limited (1) or free (0)
1b performance limiting (1=limited, 0=free) 1b E-Stop behavior (1=hold, 0=kill)
                                         | |
                    ••••               ••••
                    ||                | |
            4b unused        1b unused 1b status of command (1=success, 0=failure)
*/
uint8_t STATUS;

/**
 * Set emergency stop behavior
 *
 * @param action - the action to take in an emergency
 */
void setEStop(uint8_t action) { STATUS &= 0b11111110; STATUS |= action; }

/**
 * Get current EStop behavior
 *
 * @return current EStop behavior
 */
uint8_t getEStop() { return STATUS & 0b00000001; }

/**
 * Get current status
 *
 * @return status register
 */
uint8_t getStatus() { return STATUS; }

/**
 * Set bits in status register
 *
 * Used to set 1s in status register
 *
 * @param newFlag - flag to apply, due to |= only 1s will change status register
 */
void setStatus(uint8_t newFlag) { STATUS |= newFlag; }

/**
 * Set bits in status register
 *
 * Used to set 0s in status register
 *
 * @param newFlag - flag to apply, due to &= only 0s will change status register
 */
void clearStatus(uint8_t newFlag) { STATUS &= newFlag; }

uint32_t pui32DataTx[5], pui32DataRx[5];

/**
 * Initialize motor control.
 *
 * Initialize motor driver, including timer, PWM, SPI and GPIO.
 *
 * @param g_ui32SysClock - system clock to sync with
 */
void MotorInit(uint32_t g_ui32SysClock) {
    // variable inits
    duty = 0;
    VELO = 0;
    outputCurrent = 0;
    direction = 1;

    pos_int = 20;
    vel_int = 20;
    cur_int = 5;

    KP_angle = 1;
    KI_angle = 0.01;

    KP_velocity = 0.001;
    KI_velocity = 0.02;
    KD_velocity = 0.00;

    KP_current = 0.001;
    KI_current = 0.005;
    KD_current = 0;

    TARGET_ANGLE = 0;
    TARGET_VELO = 0;
    TARGET_CUR = 0;
    UARTprintf("initializing motor driver...\n");

    PWMInit();

    // motor pins configuration
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    // set PQ1 Always high for enable
    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1,GPIO_PIN_1);

    // set PP0 as INLC, brake if low
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_0,GPIO_PIN_0);

    // set PP1 as INHC, counterclockwise when PP1 is high
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1,GPIO_PIN_1);

    MotorSPIInit(g_ui32SysClock);
    MotorSPISetting();

    HallSensorInit();
    UARTprintf("Motor driver initialized!\n");
}

/**
 * Initialize PWM.
 */
void PWMInit(void) {
    /*********** PWM control ***********/
    // enable PWM output on PG0, which connect to INHA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    // 1/64 of system clock: 120Mhz/64
    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_1);//64

    // set PG0 as PWM pin
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

/**
 * Initialize motor control.
 *
 * Initialize motor driver, including timer, PWM, SPI and GPIO.
 *
 * @param g_ui32SysClock - system clock to sync with
 */
void MotorSPIInit(uint32_t ui32SysClock) {
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

/**
 * Initialize Hall sensor reading.
 */
void HallSensorInit(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    // Configure input pins for hall sensors
    GPIOPinTypeGPIOInput(GPIO_PORTP_BASE, GPIO_PIN_5 | GPIO_PIN_3 | GPIO_PIN_2);
    UARTprintf("Hall sensors initialized\n");
}

/**
 * Setup SPI for motor control.
 */
void MotorSPISetting(void) {
    while(SSIDataGetNonBlocking(SSI2_BASE, &pui32DataRx[0])) { }
        // set register 2h, bit 6 and 5 to 10, option 3, 1x PWM mode, bit 4 to 0,
        // synchronous
        pui32DataTx[0] = 0b1001000000000000;
        pui32DataTx[1] = 0b0001000001000000; // read register 2h, first bit is 1 for reading
        pui32DataTx[2]=  0b1011000000000000; // read register 6h
        pui32DataTx[3]=  0b0011001010000011; // read register 6h, was 01010000011
        pui32DataTx[4]=  0b1011000000000000; // read register 6h

        GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2, 0);
        SysCtlDelay(1);

        int i;
        // only reading and writing 3? times
        for(i = 0; i < 5; ++i) {
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,0); // pull FSS low
            SysCtlDelay(1);
            SSIDataPut(SSI2_BASE, pui32DataTx[i]); // send data
            SysCtlDelay(1000); // wait (at least 50ns)
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,GPIO_PIN_2); // set FSS high
            SSIDataGet(SSI2_BASE, &pui32DataRx[i]); // get data
        }

        while(SSIBusy(SSI2_BASE)) { }

    UARTprintf("register 2h before:%d:\n",pui32DataRx[0]);
    UARTprintf("register 2h after:%d:\n",pui32DataRx[1]);
    UARTprintf("register 6h before:%d:\n",pui32DataRx[2]);
    UARTprintf("register 6h after:%d:\n",pui32DataRx[3]);
    UARTprintf("register 6h after:%d:\n",pui32DataRx[4]);
    GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,GPIO_PIN_2); // Make sure pin is high

    SysCtlDelay(1000);
}

/**
 * Get the current angle reading.
 *
 * @return the current angle in degrees
 */
float getAngle(void) { return ANGLE; }

/**
 * Set the current angle.
 *
 * @param newAngle - the new angle value (in degrees) to update with
 */
void setAngle(float newAngle) { ANGLE = newAngle; }

/**
 * Get the target angle.
 *
 * @return the target angle in degrees
 */
float getTargetAngle(void) { return TARGET_ANGLE; }

/**
 * Set the target angle.
 *
 * @param newAngle - the new target angle in degrees
 */
void setTargetAngle(float newAngle) { TARGET_ANGLE = newAngle; }

/**
 * Update current angle reading from position encoder.
 */
void updateAngle(void) {
    uint32_t angle, mag, agc;
    // average data over number of cycles
    readAverageData(&angle, &mag, &agc);
    PrevAngle = getAngle();
    // calculate final angle position in degrees
    setAngle(calcFinalAngle(angle, getSection()));
}

/**
 * Get the current velocity reading.
 *
 * @return the current velocity in rps
 */
float getVelocity() { return VELO; }

/**
 * Set the current velocity.
 *
 * @param newVelocity - the new velocity value (in rps) to update with
 */
void setVelocity(float newVelocity) { VELO = newVelocity; }

/**
 * Get the target velocity.
 *
 * @return the target velocity in rps
 */
float getTargetVelocity() { return TARGET_VELO; }

/**
 * Set the target velocity.
 *
 * @param newVelocity - the new target velocity in rps
 */
void setTargetVelocity(float newVelocity) { TARGET_VELO = newVelocity; }

/**
 * Update current velocity reading.
 *
 * New velocity is difference of two angles in 1ms.
 */
void updateVelocity() {
    float diff = getAngle() - PrevAngle;
    // use acute angle
    // the motor move from 350 to 10, the diff should be 20 instead of -340, etc
    diff = diff < -180 ? diff + 360 : diff;
    diff = diff > 180 ? diff - 360 : diff;

    prevVelo = getVelocity();
    // assumes measuring velocity every 1 mili seconds
    float newVelo = diff*1000/vel_int/360*60;

    if(newVelo - prevVelo > 50) { setVelocity(prevVelo + 50); }
    else if(newVelo - prevVelo < -50) { setVelocity(prevVelo - 50); }
    else { setVelocity(newVelo); }
}

/**
 * Control PWM output to motor driver.
 *
 * @param dut - percentage of full output. Clockwise spin if positive,
 *  counterclockwise if negative
 */
void PWMoutput(float dut){
    if(dut == 0) { PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, 1); }
    else if(dut < 0) {
        dut = -dut;
        direction = -1;
        GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1, 0); // clockwise from the bottom
        PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, (int) 9600*dut/100);
    } else {
        direction = 1;
        GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_1, GPIO_PIN_1);
        PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, (int) 9600*dut/100);
    }
}

/**
 * Get PWM pulse width
 *
 * @return PWD pulse width
 */
uint32_t getPWM(void) { return PWMPulseWidthGet(PWM0_BASE, PWM_GEN_2); }

/**
 * Brake motor.
 *
 * Set pwm to 0 and nBrake to low.
 *
 * @param shouldHoldPosition - whether or not to hold current position or kill power
 */
void brake(bool shouldHoldPosition) {
    if(shouldHoldPosition) { setTargetAngle(getAngle()); }
    else { disableDriver(); }
}

/**
 * Disable motor driver.
 */
void disableDriver(void) { GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0); }

/**
 * Enable motor driver.
 */
void enableDriver(void) {
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1);
    MotorSPISetting();
}

/**
 * Control position with incredmental PID control.
 *
 * @param target - target angle to reach (unit: degrees)
 */
void PositionControl(float target) {
    float error = target - getAngle();
    error = error < -180 ? error + 360 : error;
    error = error > 180 ? error - 360 : error;
    angleErrorInt += error;
    if(angleErrorInt>=500) { angleErrorInt=500; }

    float outputVelo = KP_angle*error + KI_angle*angleErrorInt;

    TARGET_CUR=outputVelo;
    PWMoutput((int)outputVelo);
}

/**
 * Control velocity with incredmental PID control.
 *
 * @param target - target velocity to reach (unit: rps)
 */
void VelocityControl(float target) {
    float error = target - getVelocity();
    // P*(e(k) - e(k - 1)) + I*e(k) + D*((e(k) - e(k - 1)) - (e(k - 1) - e(k - 2)))
    outputCurrent += KP_velocity*(error - lastVeloError) + KI_velocity*error
        + KD_velocity*(error + prevVeloError - 2*lastVeloError);
    prevVeloError = lastVeloError;
    lastVeloError = error;

    TARGET_CUR = outputCurrent;
    PWMoutput((int) outputCurrent);
}

/**
 * Control current with incredmental PID control.
 *
 * @param target - target current to reach (unit: mA)
 */
void CurrentControl(float target) {
    // current sample rate should be consistent with the position
    float error = target - getCurrent() * direction;
    // P*(e(k) - e(k - 1)) + I*e(k) + D*((e(k) - e(k - 1)) - (e(k - 1) - e(k - 2)))
	duty += KP_current*(error - lastCurError) + KI_current*error +
        KD_current*(error + prevCurError - 2*lastCurError);
	prevCurError = lastCurError;
	lastCurError = error;
    // regulator
	if(duty < -80) { duty = -80; }
	else if(duty > 80) { duty = 80; }

   PWMoutput(duty);
}
