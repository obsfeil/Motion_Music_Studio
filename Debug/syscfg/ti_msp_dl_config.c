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
 *  DO NOT EDIT - This file is generated for the LP_MSPM0G3507
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gPWM_AUDIOBackup;
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
    SYSCFG_DL_I2C_0_init();
    SYSCFG_DL_I2C_1_init();
    SYSCFG_DL_SPI_LCD_init();
    SYSCFG_DL_ADC_MIC_JOY_init();
    SYSCFG_DL_ADC_ACCEL_init();
    SYSCFG_DL_RTC_init();
    SYSCFG_DL_SYSCTL_CLK_init();
    /* Ensure backup structures have no valid state */
	gPWM_AUDIOBackup.backupRdy 	= false;
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

	retStatus &= DL_TimerA_saveConfiguration(PWM_AUDIO_INST, &gPWM_AUDIOBackup);
	retStatus &= DL_TimerG_saveConfiguration(TIMER_SAMPLE_INST, &gTIMER_SAMPLEBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_LCD_INST, &gSPI_LCDBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(PWM_AUDIO_INST, &gPWM_AUDIOBackup, false);
	retStatus &= DL_TimerG_restoreConfiguration(TIMER_SAMPLE_INST, &gTIMER_SAMPLEBackup, false);
	retStatus &= DL_SPI_restoreConfiguration(SPI_LCD_INST, &gSPI_LCDBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(PWM_AUDIO_INST);
    DL_TimerG_reset(TIMER_SAMPLE_INST);
    DL_I2C_reset(I2C_0_INST);
    DL_I2C_reset(I2C_1_INST);
    DL_SPI_reset(SPI_LCD_INST);
    DL_ADC12_reset(ADC_MIC_JOY_INST);
    DL_ADC12_reset(ADC_ACCEL_INST);
    DL_RTC_reset(RTC);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(PWM_AUDIO_INST);
    DL_TimerG_enablePower(TIMER_SAMPLE_INST);
    DL_I2C_enablePower(I2C_0_INST);
    DL_I2C_enablePower(I2C_1_INST);
    DL_SPI_enablePower(SPI_LCD_INST);
    DL_ADC12_enablePower(ADC_MIC_JOY_INST);
    DL_ADC12_enablePower(ADC_ACCEL_INST);
    DL_RTC_enablePower(RTC);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{
    DL_GPIO_enableGlobalFastWake(GPIOA);
    DL_GPIO_enableGlobalFastWake(GPIOB);
    const uint8_t unusedPinIndexes[] =
    {
        IOMUX_PINCM30, IOMUX_PINCM31, IOMUX_PINCM32, IOMUX_PINCM33,
        IOMUX_PINCM34, IOMUX_PINCM36, IOMUX_PINCM39, IOMUX_PINCM46,
        IOMUX_PINCM48, IOMUX_PINCM49, IOMUX_PINCM51, IOMUX_PINCM52,
        IOMUX_PINCM53, IOMUX_PINCM54, IOMUX_PINCM56, IOMUX_PINCM60,
        IOMUX_PINCM3, IOMUX_PINCM5, IOMUX_PINCM6, IOMUX_PINCM8,
        IOMUX_PINCM9, IOMUX_PINCM10, IOMUX_PINCM11, IOMUX_PINCM12,
        IOMUX_PINCM14, IOMUX_PINCM15, IOMUX_PINCM16, IOMUX_PINCM18,
        IOMUX_PINCM19, IOMUX_PINCM20, IOMUX_PINCM27, IOMUX_PINCM28,
        IOMUX_PINCM29
    };

    for(int i = 0; i < sizeof(unusedPinIndexes)/sizeof(unusedPinIndexes[0]); i++)
    {
        DL_GPIO_initDigitalOutput(unusedPinIndexes[i]);
    }

    DL_GPIO_clearPins(GPIOA,
        (DL_GPIO_PIN_12 | DL_GPIO_PIN_14 | DL_GPIO_PIN_17 | DL_GPIO_PIN_21 |
        DL_GPIO_PIN_23 | DL_GPIO_PIN_24 | DL_GPIO_PIN_27 | DL_GPIO_PIN_28 |
        DL_GPIO_PIN_30 | DL_GPIO_PIN_31 | DL_GPIO_PIN_3 | DL_GPIO_PIN_4 |
        DL_GPIO_PIN_5 | DL_GPIO_PIN_6 | DL_GPIO_PIN_7 | DL_GPIO_PIN_8 |
        DL_GPIO_PIN_9));
    DL_GPIO_enableOutput(GPIOA,
        (DL_GPIO_PIN_12 | DL_GPIO_PIN_14 | DL_GPIO_PIN_17 | DL_GPIO_PIN_21 |
        DL_GPIO_PIN_23 | DL_GPIO_PIN_24 | DL_GPIO_PIN_27 | DL_GPIO_PIN_28 |
        DL_GPIO_PIN_30 | DL_GPIO_PIN_31 | DL_GPIO_PIN_3 | DL_GPIO_PIN_4 |
        DL_GPIO_PIN_5 | DL_GPIO_PIN_6 | DL_GPIO_PIN_7 | DL_GPIO_PIN_8 |
        DL_GPIO_PIN_9));
    DL_GPIO_clearPins(GPIOB,
        (DL_GPIO_PIN_13 | DL_GPIO_PIN_14 | DL_GPIO_PIN_15 | DL_GPIO_PIN_16 |
        DL_GPIO_PIN_20 | DL_GPIO_PIN_21 | DL_GPIO_PIN_23 | DL_GPIO_PIN_24 |
        DL_GPIO_PIN_25 | DL_GPIO_PIN_0 | DL_GPIO_PIN_2 | DL_GPIO_PIN_3 |
        DL_GPIO_PIN_5 | DL_GPIO_PIN_10 | DL_GPIO_PIN_11 | DL_GPIO_PIN_12));
    DL_GPIO_enableOutput(GPIOB,
        (DL_GPIO_PIN_13 | DL_GPIO_PIN_14 | DL_GPIO_PIN_15 | DL_GPIO_PIN_16 |
        DL_GPIO_PIN_20 | DL_GPIO_PIN_21 | DL_GPIO_PIN_23 | DL_GPIO_PIN_24 |
        DL_GPIO_PIN_25 | DL_GPIO_PIN_0 | DL_GPIO_PIN_2 | DL_GPIO_PIN_3 |
        DL_GPIO_PIN_5 | DL_GPIO_PIN_10 | DL_GPIO_PIN_11 | DL_GPIO_PIN_12));

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_AUDIO_C0_IOMUX,GPIO_PWM_AUDIO_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_AUDIO_C0_PORT, GPIO_PWM_AUDIO_C0_PIN);

    
	DL_GPIO_initPeripheralInputFunctionFeatures(
		 GPIO_I2C_0_IOMUX_SDA, GPIO_I2C_0_IOMUX_SDA_FUNC,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
	DL_GPIO_initPeripheralInputFunctionFeatures(
		 GPIO_I2C_0_IOMUX_SCL, GPIO_I2C_0_IOMUX_SCL_FUNC,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_I2C_0_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_I2C_0_IOMUX_SCL);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_1_IOMUX_SDA,
        GPIO_I2C_1_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_1_IOMUX_SCL,
        GPIO_I2C_1_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_I2C_1_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_I2C_1_IOMUX_SCL);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_LCD_IOMUX_CS0, GPIO_SPI_LCD_IOMUX_CS0_FUNC);
    
	DL_GPIO_initPeripheralOutputFunction(
		 GPIO_SPI_LCD_IOMUX_SCLK, GPIO_SPI_LCD_IOMUX_SCLK_FUNC);
	DL_GPIO_initPeripheralOutputFunction(
		 GPIO_SPI_LCD_IOMUX_PICO, GPIO_SPI_LCD_IOMUX_PICO_FUNC);
	DL_GPIO_initPeripheralInputFunctionFeatures(
		 GPIO_SPI_LCD_IOMUX_POCI, GPIO_SPI_LCD_IOMUX_POCI_FUNC,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    
	DL_GPIO_setAnalogInternalResistor(GPIO_ADC_MIC_JOY_IOMUX_C1, DL_GPIO_RESISTOR_NONE);
    
	DL_GPIO_setAnalogInternalResistor(GPIO_ADC_ACCEL_IOMUX_C6, DL_GPIO_RESISTOR_NONE);
	DL_GPIO_setAnalogInternalResistor(GPIO_ADC_ACCEL_IOMUX_C8, DL_GPIO_RESISTOR_NONE);
	DL_GPIO_setAnalogInternalResistor(GPIO_ADC_ACCEL_IOMUX_C5, DL_GPIO_RESISTOR_NONE);
	DL_GPIO_setAnalogInternalResistor(GPIO_ADC_ACCEL_IOMUX_C0, DL_GPIO_RESISTOR_NONE);

    DL_GPIO_initDigitalOutput(LCD_BACKLIGHT_PIN_0_IOMUX);

    DL_GPIO_initDigitalOutput(LCD_CS_PIN_LCD_CS_IOMUX);

    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_ON_0);

    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_ON_0);

    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_JOY_SEL_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_ON_0);

    DL_GPIO_initDigitalOutput(GPIO_RGB_RED_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_RGB_GREEN_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_RGB_BLUE_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_RST_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_DC_IOMUX);

    DL_GPIO_setPins(GPIOA, LCD_CS_PIN_LCD_CS_PIN |
		GPIO_LCD_DC_PIN);
    DL_GPIO_enableOutput(GPIOA, LCD_CS_PIN_LCD_CS_PIN |
		GPIO_LCD_DC_PIN);
    DL_GPIO_setLowerPinsPolarity(GPIOA, DL_GPIO_PIN_11_EDGE_FALL |
		DL_GPIO_PIN_10_EDGE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOA, DL_GPIO_PIN_18_EDGE_FALL);
    DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S1_PIN |
		GPIO_BUTTONS_S2_PIN |
		GPIO_BUTTONS_JOY_SEL_PIN);
    DL_GPIO_enableInterrupt(GPIOA, GPIO_BUTTONS_S1_PIN |
		GPIO_BUTTONS_S2_PIN |
		GPIO_BUTTONS_JOY_SEL_PIN);
    DL_GPIO_setPins(GPIOB, LCD_BACKLIGHT_PIN_0_PIN |
		GPIO_RGB_RED_PIN |
		GPIO_RGB_GREEN_PIN |
		GPIO_RGB_BLUE_PIN |
		GPIO_LCD_RST_PIN);
    DL_GPIO_enableOutput(GPIOB, LCD_BACKLIGHT_PIN_0_PIN |
		GPIO_RGB_RED_PIN |
		GPIO_RGB_GREEN_PIN |
		GPIO_RGB_BLUE_PIN |
		GPIO_LCD_RST_PIN);

}


static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_16_32_MHZ,
	.rDivClk2x              = 9,
	.rDivClk1               = 0,
	.rDivClk0               = 0,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_ENABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_ENABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_ENABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK0,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_SYSOSC,
	.qDiv                   = 9,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_2,
	
};

SYSCONFIG_WEAK bool SYSCFG_DL_SYSCTL_SYSPLL_init(void)
{
    bool fFCCRatioStatus = false;
    uint32_t fFCCSysoscCount;
    uint32_t fFCCPllCount;
    uint32_t fFCCRatio;

    DL_SYSCTL_setFCCPeriods( DL_SYSCTL_FCC_TRIG_CNT_01 );

    /* Measuring PLL. */
    DL_SYSCTL_configFCC(DL_SYSCTL_FCC_TRIG_TYPE_RISE_RISE,
                        DL_SYSCTL_FCC_TRIG_SOURCE_LFCLK,
                        DL_SYSCTL_FCC_CLOCK_SOURCE_SYSPLLCLK0);
    /* Get SYSPLL frequency using FCC */
    DL_SYSCTL_startFCC();
    while (DL_SYSCTL_isFCCDone() == 0);

    /* get measA= SYSPLLCLK0 freq wrt LFOSC*/
    fFCCPllCount = DL_SYSCTL_readFCC();

    /* Measuring SYSPLL Source */
    DL_SYSCTL_configFCC(DL_SYSCTL_FCC_TRIG_TYPE_RISE_RISE,
                        DL_SYSCTL_FCC_TRIG_SOURCE_LFCLK,
                        DL_SYSCTL_FCC_CLOCK_SOURCE_SYSOSC);
    /* Get SYSPLL frequency using FCC */
    DL_SYSCTL_startFCC();
    while (DL_SYSCTL_isFCCDone() == 0 );

    /* get measB= SYSOSC freq wrt LFOSC*/
    fFCCSysoscCount = DL_SYSCTL_readFCC();

    /* Get ratio of both measurements*/
    fFCCRatio = (fFCCPllCount * FLOAT_TO_INT_SCALE) / fFCCSysoscCount;
    /* Check ratio is within bounds*/
    if ((FCC_LOWER_BOUND <  fFCCRatio) && (fFCCRatio < FCC_UPPER_BOUND))
    {
        /* ratio is good for proceeding into application code. */
        fFCCRatioStatus = true;
    }

    return fFCCRatioStatus;
}
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be STANDBY0
    DL_SYSCTL_setPowerPolicySTANDBY0();
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setVBOOSTConfig(DL_SYSCTL_VBOOST_ONALWAYS);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* Set default configuration */
    DL_SYSCTL_disableHFXT();
    DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *) &gSYSPLLConfig);
	
    /*
     * [SYSPLL_ERR_01]
     * PLL Incorrect locking WA start.
     * Insert after every PLL enable.
     * This can lead an infinite loop if the condition persists
     * and can block entry to the application code.
     */

    while (SYSCFG_DL_SYSCTL_SYSPLL_init() == false)
    {
        /* Toggle SYSPLL enable to re-enable SYSPLL and re-check incorrect locking */
        DL_SYSCTL_disableSYSPLL();
        DL_SYSCTL_enableSYSPLL();

        /* Wait until SYSPLL startup is stabilized*/
        while ((DL_SYSCTL_getClockStatus() & SYSCTL_CLKSTATUS_SYSPLLGOOD_MASK) != DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD){}
    }
    DL_SYSCTL_enableMFCLK();
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
    DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);
    DL_SYSCTL_enableMFPCLK();
    /* Enable interrupt for Flash Command execution is complete */
    DL_FlashCTL_enableInterrupt(FLASHCTL);
    DL_SYSCTL_enableInterrupt((DL_SYSCTL_INTERRUPT_HSCLK_GOOD
		 | DL_SYSCTL_INTERRUPT_SYSPLL_GOOD
		 | DL_SYSCTL_INTERRUPT_FLASH_SEC));
    /* INT_GROUP0 Priority */
    NVIC_SetPriority(FLASHCTL_INT_IRQn, 0);

}
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_CLK_init(void) {
    while ((DL_SYSCTL_getClockStatus() & (DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD
		 | DL_SYSCTL_CLK_STATUS_HSCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_LFOSC_GOOD))
	       != (DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD
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
static const DL_TimerA_ClockConfig gPWM_AUDIOClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerA_PWMConfig gPWM_AUDIOConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 4095,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_AUDIO_init(void) {

    DL_TimerA_setClockConfig(
        PWM_AUDIO_INST, (DL_TimerA_ClockConfig *) &gPWM_AUDIOClockConfig);

    DL_TimerA_initPWMMode(
        PWM_AUDIO_INST, (DL_TimerA_PWMConfig *) &gPWM_AUDIOConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerA_setCounterControl(PWM_AUDIO_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerA_setCaptureCompareOutCtl(PWM_AUDIO_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_0_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_AUDIO_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_AUDIO_INST, 2047, DL_TIMER_CC_0_INDEX);

    DL_TimerA_enableClock(PWM_AUDIO_INST);


    DL_TimerA_enableInterrupt(PWM_AUDIO_INST , DL_TIMER_INTERRUPT_ZERO_EVENT);

    DL_TimerA_setCCPDirection(PWM_AUDIO_INST , DL_TIMER_CC0_OUTPUT );


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
 * TIMER_SAMPLE_INST_LOAD_VALUE = (125 us * 80000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_SAMPLETimerConfig = {
    .period     = TIMER_SAMPLE_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC_UP,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_SAMPLE_init(void) {

    DL_TimerG_setClockConfig(TIMER_SAMPLE_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_SAMPLEClockConfig);

    DL_TimerG_initTimerMode(TIMER_SAMPLE_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_SAMPLETimerConfig);
    DL_TimerG_enableInterrupt(TIMER_SAMPLE_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
	NVIC_SetPriority(TIMER_SAMPLE_INST_INT_IRQN, 0);
    DL_TimerG_enableClock(TIMER_SAMPLE_INST);


    DL_TimerG_enableEvent(TIMER_SAMPLE_INST, DL_TIMERG_EVENT_ROUTE_1, (DL_TIMERG_EVENT_ZERO_EVENT));

    DL_TimerG_setPublisherChanID(TIMER_SAMPLE_INST, DL_TIMERG_PUBLISHER_INDEX_0, TIMER_SAMPLE_INST_PUB_0_CH);



}


static const DL_I2C_ClockConfig gI2C_0ClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_I2C_0_init(void) {

    DL_I2C_setClockConfig(I2C_0_INST,
        (DL_I2C_ClockConfig *) &gI2C_0ClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(I2C_0_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(I2C_0_INST);

    /* Configure Controller Mode */
    DL_I2C_resetControllerTransfer(I2C_0_INST);
    /* Set frequency to 100000 Hz*/
    DL_I2C_setTimerPeriod(I2C_0_INST, 39);
    DL_I2C_setControllerTXFIFOThreshold(I2C_0_INST, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(I2C_0_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(I2C_0_INST);


    /* Enable module */
    DL_I2C_enableController(I2C_0_INST);


}
static const DL_I2C_ClockConfig gI2C_1ClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_I2C_1_init(void) {

    DL_I2C_setClockConfig(I2C_1_INST,
        (DL_I2C_ClockConfig *) &gI2C_1ClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(I2C_1_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(I2C_1_INST);

    /* Configure Controller Mode */
    DL_I2C_resetControllerTransfer(I2C_1_INST);
    /* Set frequency to 100000 Hz*/
    DL_I2C_setTimerPeriod(I2C_1_INST, 39);
    DL_I2C_setControllerTXFIFOThreshold(I2C_1_INST, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(I2C_1_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(I2C_1_INST);


    /* Enable module */
    DL_I2C_enableController(I2C_1_INST);


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
     *     8000000 = (80000000)/((1 + 4) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_LCD_INST, 4);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_LCD_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_LCD_INST);
}

/* ADC_MIC_JOY Initialization */
static const DL_ADC12_ClockConfig gADC_MIC_JOYClockConfig = {
    .clockSel       = DL_ADC12_CLOCK_SYSOSC,
    .divideRatio    = DL_ADC12_CLOCK_DIVIDE_1,
    .freqRange      = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
};
SYSCONFIG_WEAK void SYSCFG_DL_ADC_MIC_JOY_init(void)
{
    DL_ADC12_setClockConfig(ADC_MIC_JOY_INST, (DL_ADC12_ClockConfig *) &gADC_MIC_JOYClockConfig);

    DL_ADC12_initSeqSample(ADC_MIC_JOY_INST,
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_EVENT,
        DL_ADC12_SEQ_START_ADDR_00, DL_ADC12_SEQ_END_ADDR_01, DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(ADC_MIC_JOY_INST, ADC_MIC_JOY_ADCMEM_0,
        DL_ADC12_INPUT_CHAN_1, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_MIC_JOY_INST, ADC_MIC_JOY_ADCMEM_1,
        DL_ADC12_INPUT_CHAN_2, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_setSampleTime0(ADC_MIC_JOY_INST,4000);
    DL_ADC12_setSubscriberChanID(ADC_MIC_JOY_INST,ADC_MIC_JOY_INST_SUB_CH);
    /* Enable ADC12 interrupt */
    DL_ADC12_clearInterruptStatus(ADC_MIC_JOY_INST,(DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED));
    DL_ADC12_enableInterrupt(ADC_MIC_JOY_INST,(DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED
		 | DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED));
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
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_SOFTWARE,
        DL_ADC12_SEQ_START_ADDR_00, DL_ADC12_SEQ_END_ADDR_03, DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_0,
        DL_ADC12_INPUT_CHAN_6, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_1,
        DL_ADC12_INPUT_CHAN_8, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_2,
        DL_ADC12_INPUT_CHAN_5, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_configConversionMem(ADC_ACCEL_INST, ADC_ACCEL_ADCMEM_3,
        DL_ADC12_INPUT_CHAN_0, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_AUTO_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_setSampleTime0(ADC_ACCEL_INST,500);
    /* Enable ADC12 interrupt */
    DL_ADC12_clearInterruptStatus(ADC_ACCEL_INST,(DL_ADC12_INTERRUPT_MEM3_RESULT_LOADED));
    DL_ADC12_enableInterrupt(ADC_ACCEL_INST,(DL_ADC12_INTERRUPT_MEM3_RESULT_LOADED));
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
}

static const DL_RTC_Calendar gRTCCalendarConfig = {
		.seconds    = 0,   /* Seconds = 0 */
		.minutes    = 0,   /* Minute = 0 */
		.hours      = 0,   /* Hour = 0 */
		.dayOfWeek  = 0,    /* Day of week = 0 (Sunday) */
		.dayOfMonth = 1,    /* Day of month = 1*/
		.month      = 1,    /* Month = 1 (January) */
		.year       = 2022, /* Year = 2022 */
};




SYSCONFIG_WEAK void SYSCFG_DL_RTC_init(void)
{
	/* Initialize RTC Calendar */
	DL_RTC_initCalendar(RTC , gRTCCalendarConfig, DL_RTC_FORMAT_BINARY);

}

