/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerG_backupConfig gTIMER_SAMPLEBackup;
DL_SPI_backupConfig gSPI_LCDBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_PWM_AUDIO_init();
    SYSCFG_DL_TIMER_SAMPLE_init();
    SYSCFG_DL_I2C_SENSORS_init();
    SYSCFG_DL_SPI_LCD_init();
    SYSCFG_DL_ADC_MIC_JOY_init();
    SYSCFG_DL_ADC_ACCEL_init();
    SYSCFG_DL_VREF_init();
    SYSCFG_DL_SYSCTL_CLK_init();
    /* Ensure backup structures have no valid state */

	gTIMER_SAMPLEBackup.backupRdy 	= false;
	gSPI_LCDBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_saveConfiguration(TIMER_SAMPLE_INST, &gTIMER_SAMPLEBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_LCD_INST, &gSPI_LCDBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_restoreConfiguration(TIMER_SAMPLE_INST, &gTIMER_SAMPLEBackup, false);
	retStatus &= DL_SPI_restoreConfiguration(SPI_LCD_INST, &gSPI_LCDBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(PWM_AUDIO_INST);
    DL_TimerG_reset(TIMER_SAMPLE_INST);
    DL_I2C_reset(I2C_SENSORS_INST);
    DL_SPI_reset(SPI_LCD_INST);
    DL_ADC12_reset(ADC_MIC_JOY_INST);
    DL_ADC12_reset(ADC_ACCEL_INST);
    DL_VREF_reset(VREF);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(PWM_AUDIO_INST);
    DL_TimerG_enablePower(TIMER_SAMPLE_INST);
    DL_I2C_enablePower(I2C_SENSORS_INST);
    DL_SPI_enablePower(SPI_LCD_INST);
    DL_ADC12_enablePower(ADC_MIC_JOY_INST);
    DL_ADC12_enablePower(ADC_ACCEL_INST);
    DL_VREF_enablePower(VREF);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{
    const uint8_t unusedPinIndexes[] =
    {
        IOMUX_PINCM30, IOMUX_PINCM31, IOMUX_PINCM35, IOMUX_PINCM47,
        IOMUX_PINCM48, IOMUX_PINCM49, IOMUX_PINCM50, IOMUX_PINCM51,
        IOMUX_PINCM52, IOMUX_PINCM54, IOMUX_PINCM56, IOMUX_PINCM57,
        IOMUX_PINCM58, IOMUX_PINCM3, IOMUX_PINCM4, IOMUX_PINCM5,
        IOMUX_PINCM6, IOMUX_PINCM7, IOMUX_PINCM8, IOMUX_PINCM9,
        IOMUX_PINCM10, IOMUX_PINCM12, IOMUX_PINCM13, IOMUX_PINCM14,
        IOMUX_PINCM15, IOMUX_PINCM16, IOMUX_PINCM17, IOMUX_PINCM18,
        IOMUX_PINCM19, IOMUX_PINCM20, IOMUX_PINCM21, IOMUX_PINCM22,
        IOMUX_PINCM27, IOMUX_PINCM28, IOMUX_PINCM29
    };

    for(int i = 0; i < sizeof(unusedPinIndexes)/sizeof(unusedPinIndexes[0]); i++)
    {
        DL_GPIO_initDigitalOutput(unusedPinIndexes[i]);
    }

    DL_GPIO_clearPins(GPIOA,
        (DL_GPIO_PIN_13 | DL_GPIO_PIN_22 | DL_GPIO_PIN_24 | DL_GPIO_PIN_28 |
        DL_GPIO_PIN_29 | DL_GPIO_PIN_30 | DL_GPIO_PIN_31 | DL_GPIO_PIN_2 |
        DL_GPIO_PIN_3 | DL_GPIO_PIN_4 | DL_GPIO_PIN_5 | DL_GPIO_PIN_7 |
        DL_GPIO_PIN_8 | DL_GPIO_PIN_9 | DL_GPIO_PIN_10 | DL_GPIO_PIN_11));
    DL_GPIO_enableOutput(GPIOA,
        (DL_GPIO_PIN_13 | DL_GPIO_PIN_22 | DL_GPIO_PIN_24 | DL_GPIO_PIN_28 |
        DL_GPIO_PIN_29 | DL_GPIO_PIN_30 | DL_GPIO_PIN_31 | DL_GPIO_PIN_2 |
        DL_GPIO_PIN_3 | DL_GPIO_PIN_4 | DL_GPIO_PIN_5 | DL_GPIO_PIN_7 |
        DL_GPIO_PIN_8 | DL_GPIO_PIN_9 | DL_GPIO_PIN_10 | DL_GPIO_PIN_11));
    DL_GPIO_clearPins(GPIOB,
        (DL_GPIO_PIN_13 | DL_GPIO_PIN_14 | DL_GPIO_PIN_20 | DL_GPIO_PIN_21 |
        DL_GPIO_PIN_22 | DL_GPIO_PIN_23 | DL_GPIO_PIN_24 | DL_GPIO_PIN_25 |
        DL_GPIO_PIN_26 | DL_GPIO_PIN_27 | DL_GPIO_PIN_0 | DL_GPIO_PIN_1 |
        DL_GPIO_PIN_2 | DL_GPIO_PIN_3 | DL_GPIO_PIN_4 | DL_GPIO_PIN_5 |
        DL_GPIO_PIN_10 | DL_GPIO_PIN_11 | DL_GPIO_PIN_12));
    DL_GPIO_enableOutput(GPIOB,
        (DL_GPIO_PIN_13 | DL_GPIO_PIN_14 | DL_GPIO_PIN_20 | DL_GPIO_PIN_21 |
        DL_GPIO_PIN_22 | DL_GPIO_PIN_23 | DL_GPIO_PIN_24 | DL_GPIO_PIN_25 |
        DL_GPIO_PIN_26 | DL_GPIO_PIN_27 | DL_GPIO_PIN_0 | DL_GPIO_PIN_1 |
        DL_GPIO_PIN_2 | DL_GPIO_PIN_3 | DL_GPIO_PIN_4 | DL_GPIO_PIN_5 |
        DL_GPIO_PIN_10 | DL_GPIO_PIN_11 | DL_GPIO_PIN_12));

    DL_GPIO_initPeripheralInputFunction(GPIO_HFCLKIN_IOMUX, GPIO_HFCLKIN_IOMUX_FUNC);

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_AUDIO_C0_IOMUX,GPIO_PWM_AUDIO_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_AUDIO_C0_PORT, GPIO_PWM_AUDIO_C0_PIN);

    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_SENSORS_IOMUX_SDA,
        GPIO_I2C_SENSORS_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_SENSORS_IOMUX_SCL,
        GPIO_I2C_SENSORS_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_I2C_SENSORS_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_I2C_SENSORS_IOMUX_SCL);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_LCD_IOMUX_SCLK, GPIO_SPI_LCD_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_LCD_IOMUX_PICO, GPIO_SPI_LCD_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SPI_LCD_IOMUX_POCI, GPIO_SPI_LCD_IOMUX_POCI_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_LCD_IOMUX_CS0, GPIO_SPI_LCD_IOMUX_CS0_FUNC);

    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_JOY_SEL_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(GPIO_RGB_RED_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_RGB_GREEN_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_RGB_BLUE_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_RST_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_DC_IOMUX);

    DL_GPIO_setLowerPinsPolarity(GPIO_BUTTONS_PORT, DL_GPIO_PIN_14_EDGE_FALL |
		DL_GPIO_PIN_15_EDGE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIO_BUTTONS_PORT, DL_GPIO_PIN_16_EDGE_FALL);
    DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN |
		GPIO_BUTTONS_S2_PIN |
		GPIO_BUTTONS_JOY_SEL_PIN);
    DL_GPIO_enableInterrupt(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN |
		GPIO_BUTTONS_S2_PIN |
		GPIO_BUTTONS_JOY_SEL_PIN);
    DL_GPIO_clearPins(GPIOB, GPIO_LCD_RST_PIN |
		GPIO_LCD_DC_PIN);
    DL_GPIO_setPins(GPIOB, GPIO_RGB_RED_PIN |
		GPIO_RGB_GREEN_PIN |
		GPIO_RGB_BLUE_PIN);
    DL_GPIO_enableOutput(GPIOB, GPIO_RGB_RED_PIN |
		GPIO_RGB_GREEN_PIN |
		GPIO_RGB_BLUE_PIN |
		GPIO_LCD_RST_PIN |
		GPIO_LCD_DC_PIN);

}


static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,
	.rDivClk2x              = 0,
	.rDivClk1               = 0,
	.rDivClk0               = 1,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_DISABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_DISABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_ENABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK0,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_SYSOSC,
	.qDiv                   = 9,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_1,
	
};
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be STANDBY0
    DL_SYSCTL_setPowerPolicySTANDBY0();
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* Set default configuration */
    DL_SYSCTL_disableHFXT();
    DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setHFCLKSourceHFCLKIN();
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *) &gSYSPLLConfig);
	
    DL_SYSCTL_enableMFCLK();
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_1);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
    DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_HFCLK);
    DL_SYSCTL_setHFCLKDividerForMFPCLK(DL_SYSCTL_HFCLK_MFPCLK_DIVIDER_2);
    DL_SYSCTL_enableMFPCLK();
    /* Enable interrupt for Flash Command execution is complete */
    DL_FlashCTL_enableInterrupt(FLASHCTL);
    DL_SYSCTL_enableInterrupt((DL_SYSCTL_INTERRUPT_HFCLK_GOOD
		 | DL_SYSCTL_INTERRUPT_FLASH_SEC));
    /* INT_GROUP0 Priority */
    NVIC_SetPriority(SYSCTL_INT_IRQn, 0);

}
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_CLK_init(void) {
    while ((DL_SYSCTL_getClockStatus() & (DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD
		 | DL_SYSCTL_CLK_STATUS_HFCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_HSCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_LFOSC_GOOD))
	       != (DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD
		 | DL_SYSCTL_CLK_STATUS_HFCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_HSCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_LFOSC_GOOD))
	{
		/* Ensure that clocks are in default POR configuration before initialization.
		* Additionally once LFXT is enabled, the internal LFOSC is disabled, and cannot
		* be re-enabled other than by executing a BOOTRST. */
		;
	}
}



/*
 * Timer clock configuration to be sourced by  / 1 (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   80000000 Hz = 80000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_AUDIOClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_AUDIOConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 4095,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_AUDIO_init(void) {

    DL_TimerG_setClockConfig(
        PWM_AUDIO_INST, (DL_TimerG_ClockConfig *) &gPWM_AUDIOClockConfig);

    DL_TimerG_initPWMMode(
        PWM_AUDIO_INST, (DL_TimerG_PWMConfig *) &gPWM_AUDIOConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(PWM_AUDIO_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(PWM_AUDIO_INST, DL_TIMER_CC_OCTL_INIT_VAL_HIGH,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_AUDIO_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2047, DL_TIMER_CC_0_INDEX);

    DL_TimerG_enableClock(PWM_AUDIO_INST);


    DL_TimerG_enableInterrupt(PWM_AUDIO_INST , DL_TIMER_INTERRUPT_ZERO_EVENT);

    DL_TimerG_setCCPDirection(PWM_AUDIO_INST , DL_TIMER_CC0_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   80000000 Hz = 80000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_SAMPLEClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_SAMPLE_INST_LOAD_VALUE = (50 us * 80000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_SAMPLETimerConfig = {
    .period     = TIMER_SAMPLE_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_SAMPLE_init(void) {

    DL_TimerG_setClockConfig(TIMER_SAMPLE_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_SAMPLEClockConfig);

    DL_TimerG_initTimerMode(TIMER_SAMPLE_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_SAMPLETimerConfig);
    DL_TimerG_enableInterrupt(TIMER_SAMPLE_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
	NVIC_SetPriority(TIMER_SAMPLE_INST_INT_IRQN, 1);
    DL_TimerG_enableClock(TIMER_SAMPLE_INST);





}


static const DL_I2C_ClockConfig gI2C_SENSORSClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_I2C_SENSORS_init(void) {

    DL_I2C_setClockConfig(I2C_SENSORS_INST,
        (DL_I2C_ClockConfig *) &gI2C_SENSORSClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(I2C_SENSORS_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(I2C_SENSORS_INST);

    /* Configure Controller Mode */
    DL_I2C_resetControllerTransfer(I2C_SENSORS_INST);
    /* Set frequency to 100000 Hz*/
    DL_I2C_setTimerPeriod(I2C_SENSORS_INST, 79);
    DL_I2C_setControllerTXFIFOThreshold(I2C_SENSORS_INST, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(I2C_SENSORS_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(I2C_SENSORS_INST);


    /* Enable module */
    DL_I2C_enableController(I2C_SENSORS_INST);


}

static const DL_SPI_Config gSPI_LCD_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO4_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
    .chipSelectPin = DL_SPI_CHIP_SELECT_0,
};

static const DL_SPI_ClockConfig gSPI_LCD_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_LCD_init(void) {
    DL_SPI_setClockConfig(SPI_LCD_INST, (DL_SPI_ClockConfig *) &gSPI_LCD_clockConfig);

    DL_SPI_init(SPI_LCD_INST, (DL_SPI_Config *) &gSPI_LCD_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     10000000 = (80000000)/((1 + 3) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_LCD_INST, 3);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_LCD_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_LCD_INST);
}

/* ADC_MIC_JOY Initialization */
static const DL_ADC12_ClockConfig gADC_MIC_JOYClockConfig = {
    .clockSel       = DL_ADC12_CLOCK_SYSOSC,
    .divideRatio    = DL_ADC12_CLOCK_DIVIDE_4,
    .freqRange      = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
};
SYSCONFIG_WEAK void SYSCFG_DL_ADC_MIC_JOY_init(void)
{
    DL_ADC12_setClockConfig(ADC_MIC_JOY_INST, (DL_ADC12_ClockConfig *) &gADC_MIC_JOYClockConfig);

    DL_ADC12_initSeqSample(ADC_MIC_JOY_INST,
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_SOFTWARE,
        DL_ADC12_SEQ_START_ADDR_00, DL_ADC12_SEQ_END_ADDR_02, DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(ADC_MIC_JOY_INST, ADC_MIC_JOY_ADCMEM_MIC,
        DL_ADC12_INPUT_CHAN_2, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_MIC_JOY_INST, ADC_MIC_JOY_ADCMEM_JOY_Y,
        DL_ADC12_INPUT_CHAN_1, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_MIC_JOY_INST, ADC_MIC_JOY_ADCMEM_JOY_X,
        DL_ADC12_INPUT_CHAN_0, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_setSampleTime0(ADC_MIC_JOY_INST,1000);
    DL_ADC12_setPublisherChanID(ADC_MIC_JOY_INST,ADC_MIC_JOY_INST_PUB_CH);
    DL_ADC12_setSubscriberChanID(ADC_MIC_JOY_INST,ADC_MIC_JOY_INST_SUB_CH);
    DL_ADC12_enableEvent(ADC_MIC_JOY_INST,(DL_ADC12_EVENT_MEM0_RESULT_LOADED));
    /* Enable ADC12 interrupt */
    DL_ADC12_clearInterruptStatus(ADC_MIC_JOY_INST,(DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED));
    DL_ADC12_enableInterrupt(ADC_MIC_JOY_INST,(DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED));
    DL_ADC12_enableConversions(ADC_MIC_JOY_INST);
}
/* ADC_ACCEL Initialization */
static const DL_ADC12_ClockConfig gADC_ACCELClockConfig = {
    .clockSel       = DL_ADC12_CLOCK_SYSOSC,
    .divideRatio    = DL_ADC12_CLOCK_DIVIDE_8,
    .freqRange      = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
};
SYSCONFIG_WEAK void SYSCFG_DL_ADC_ACCEL_init(void)
{
    DL_ADC12_setClockConfig(ADC_ACCEL_INST, (DL_ADC12_ClockConfig *) &gADC_ACCELClockConfig);

    DL_ADC12_initSeqSample(ADC_ACCEL_INST,
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_EVENT,
        DL_ADC12_SEQ_START_ADDR_00, DL_ADC12_SEQ_END_ADDR_02, DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_ACCEL_X,
        DL_ADC12_INPUT_CHAN_2, DL_ADC12_REFERENCE_VOLTAGE_INTREF, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_ACCEL_Y,
        DL_ADC12_INPUT_CHAN_3, DL_ADC12_REFERENCE_VOLTAGE_INTREF, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_ACCEL_Z,
        DL_ADC12_INPUT_CHAN_7, DL_ADC12_REFERENCE_VOLTAGE_INTREF, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_setSampleTime0(ADC_ACCEL_INST,500);
    DL_ADC12_setPublisherChanID(ADC_ACCEL_INST,ADC_ACCEL_INST_PUB_CH);
    DL_ADC12_setSubscriberChanID(ADC_ACCEL_INST,ADC_ACCEL_INST_SUB_CH);
    DL_ADC12_enableEvent(ADC_ACCEL_INST,(DL_ADC12_EVENT_MEM0_RESULT_LOADED));
    /* Enable ADC12 interrupt */
    DL_ADC12_clearInterruptStatus(ADC_ACCEL_INST,(DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED));
    DL_ADC12_enableInterrupt(ADC_ACCEL_INST,(DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED));
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
}


static const DL_VREF_Config gVREFConfig = {
    .vrefEnable     = DL_VREF_ENABLE_ENABLE,
    .bufConfig      = DL_VREF_BUFCONFIG_OUTPUT_2_5V,
    .shModeEnable   = DL_VREF_SHMODE_DISABLE,
    .holdCycleCount = DL_VREF_HOLD_MIN,
    .shCycleCount   = DL_VREF_SH_MIN,
};

SYSCONFIG_WEAK void SYSCFG_DL_VREF_init(void) {
    DL_VREF_configReference(VREF,
        (DL_VREF_Config *) &gVREFConfig);
    delay_cycles(VREF_READY_DELAY);
}


