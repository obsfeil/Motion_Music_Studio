/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the LP_MSPM0G3507
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_LP_MSPM0G3507
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2500
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for PWM_AUDIO */
#define PWM_AUDIO_INST                                                     TIMA1
#define PWM_AUDIO_INST_IRQHandler                               TIMA1_IRQHandler
#define PWM_AUDIO_INST_INT_IRQN                                 (TIMA1_INT_IRQn)
#define PWM_AUDIO_INST_CLK_FREQ                                         80000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_AUDIO_C0_PORT                                             GPIOB
#define GPIO_PWM_AUDIO_C0_PIN                                      DL_GPIO_PIN_4
#define GPIO_PWM_AUDIO_C0_IOMUX                                  (IOMUX_PINCM17)
#define GPIO_PWM_AUDIO_C0_IOMUX_FUNC                 IOMUX_PINCM17_PF_TIMA1_CCP0
#define GPIO_PWM_AUDIO_C0_IDX                                DL_TIMER_CC_0_INDEX



/* Defines for TIMER_SAMPLE */
#define TIMER_SAMPLE_INST                                                (TIMG7)
#define TIMER_SAMPLE_INST_IRQHandler                            TIMG7_IRQHandler
#define TIMER_SAMPLE_INST_INT_IRQN                              (TIMG7_INT_IRQn)
#define TIMER_SAMPLE_INST_LOAD_VALUE                                     (9999U)
#define TIMER_SAMPLE_INST_PUB_0_CH                                           (1)




/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define I2C_0_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                         DL_GPIO_PIN_0
#define GPIO_I2C_0_IOMUX_SDA                                      (IOMUX_PINCM1)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                       IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                         DL_GPIO_PIN_1
#define GPIO_I2C_0_IOMUX_SCL                                      (IOMUX_PINCM2)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                       IOMUX_PINCM2_PF_I2C0_SCL

/* Defines for I2C_1 */
#define I2C_1_INST                                                          I2C1
#define I2C_1_INST_IRQHandler                                    I2C1_IRQHandler
#define I2C_1_INST_INT_IRQN                                        I2C1_INT_IRQn
#define I2C_1_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_1_SDA_PORT                                                GPIOA
#define GPIO_I2C_1_SDA_PIN                                        DL_GPIO_PIN_16
#define GPIO_I2C_1_IOMUX_SDA                                     (IOMUX_PINCM38)
#define GPIO_I2C_1_IOMUX_SDA_FUNC                      IOMUX_PINCM38_PF_I2C1_SDA
#define GPIO_I2C_1_SCL_PORT                                                GPIOA
#define GPIO_I2C_1_SCL_PIN                                        DL_GPIO_PIN_29
#define GPIO_I2C_1_IOMUX_SCL                                      (IOMUX_PINCM4)
#define GPIO_I2C_1_IOMUX_SCL_FUNC                       IOMUX_PINCM4_PF_I2C1_SCL


/* Defines for SPI_LCD */
#define SPI_LCD_INST                                                       SPI1
#define SPI_LCD_INST_IRQHandler                                 SPI1_IRQHandler
#define SPI_LCD_INST_INT_IRQN                                     SPI1_INT_IRQn
#define GPIO_SPI_LCD_PICO_PORT                                            GPIOB
#define GPIO_SPI_LCD_PICO_PIN                                     DL_GPIO_PIN_8
#define GPIO_SPI_LCD_IOMUX_PICO                                 (IOMUX_PINCM25)
#define GPIO_SPI_LCD_IOMUX_PICO_FUNC                 IOMUX_PINCM25_PF_SPI1_PICO
#define GPIO_SPI_LCD_POCI_PORT                                            GPIOB
#define GPIO_SPI_LCD_POCI_PIN                                     DL_GPIO_PIN_7
#define GPIO_SPI_LCD_IOMUX_POCI                                 (IOMUX_PINCM24)
#define GPIO_SPI_LCD_IOMUX_POCI_FUNC                 IOMUX_PINCM24_PF_SPI1_POCI
/* GPIO configuration for SPI_LCD */
#define GPIO_SPI_LCD_SCLK_PORT                                            GPIOB
#define GPIO_SPI_LCD_SCLK_PIN                                     DL_GPIO_PIN_9
#define GPIO_SPI_LCD_IOMUX_SCLK                                 (IOMUX_PINCM26)
#define GPIO_SPI_LCD_IOMUX_SCLK_FUNC                 IOMUX_PINCM26_PF_SPI1_SCLK
#define GPIO_SPI_LCD_CS0_PORT                                             GPIOB
#define GPIO_SPI_LCD_CS0_PIN                                      DL_GPIO_PIN_6
#define GPIO_SPI_LCD_IOMUX_CS0                                  (IOMUX_PINCM23)
#define GPIO_SPI_LCD_IOMUX_CS0_FUNC                   IOMUX_PINCM23_PF_SPI1_CS0



/* Defines for ADC_MIC_JOY */
#define ADC_MIC_JOY_INST                                                    ADC0
#define ADC_MIC_JOY_INST_IRQHandler                              ADC0_IRQHandler
#define ADC_MIC_JOY_INST_INT_IRQN                                (ADC0_INT_IRQn)
#define ADC_MIC_JOY_ADCMEM_0                                  DL_ADC12_MEM_IDX_0
#define ADC_MIC_JOY_ADCMEM_0_REF                 DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_MIC_JOY_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define ADC_MIC_JOY_ADCMEM_1                                  DL_ADC12_MEM_IDX_1
#define ADC_MIC_JOY_ADCMEM_1_REF                 DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_MIC_JOY_ADCMEM_1_REF_VOLTAGE_V                                     3.3
#define ADC_MIC_JOY_INST_SUB_CH                                              (1)
#define GPIO_ADC_MIC_JOY_C1_PORT                                           GPIOA
#define GPIO_ADC_MIC_JOY_C1_PIN                                   DL_GPIO_PIN_26
#define GPIO_ADC_MIC_JOY_IOMUX_C1                                (IOMUX_PINCM59)
#define GPIO_ADC_MIC_JOY_IOMUX_C1_FUNC            (IOMUX_PINCM59_PF_UNCONNECTED)
#define GPIO_ADC_MIC_JOY_C2_PORT                                           GPIOA
#define GPIO_ADC_MIC_JOY_C2_PIN                                   DL_GPIO_PIN_25
#define GPIO_ADC_MIC_JOY_IOMUX_C2                                (IOMUX_PINCM55)
#define GPIO_ADC_MIC_JOY_IOMUX_C2_FUNC            (IOMUX_PINCM55_PF_UNCONNECTED)

/* Defines for ADC_ACCEL */
#define ADC_ACCEL_INST                                                      ADC1
#define ADC_ACCEL_INST_IRQHandler                                ADC1_IRQHandler
#define ADC_ACCEL_INST_INT_IRQN                                  (ADC1_INT_IRQn)
#define ADC_ACCEL_ADCMEM_0                                    DL_ADC12_MEM_IDX_0
#define ADC_ACCEL_ADCMEM_0_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_ACCEL_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define ADC_ACCEL_ADCMEM_1                                    DL_ADC12_MEM_IDX_1
#define ADC_ACCEL_ADCMEM_1_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_ACCEL_ADCMEM_1_REF_VOLTAGE_V                                     3.3
#define ADC_ACCEL_ADCMEM_2                                    DL_ADC12_MEM_IDX_2
#define ADC_ACCEL_ADCMEM_2_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_ACCEL_ADCMEM_2_REF_VOLTAGE_V                                     3.3
#define ADC_ACCEL_ADCMEM_3                                    DL_ADC12_MEM_IDX_3
#define ADC_ACCEL_ADCMEM_3_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_ACCEL_ADCMEM_3_REF_VOLTAGE_V                                     3.3
#define GPIO_ADC_ACCEL_C6_PORT                                             GPIOB
#define GPIO_ADC_ACCEL_C6_PIN                                     DL_GPIO_PIN_19
#define GPIO_ADC_ACCEL_IOMUX_C6                                  (IOMUX_PINCM45)
#define GPIO_ADC_ACCEL_IOMUX_C6_FUNC              (IOMUX_PINCM45_PF_UNCONNECTED)
#define GPIO_ADC_ACCEL_C8_PORT                                             GPIOA
#define GPIO_ADC_ACCEL_C8_PIN                                     DL_GPIO_PIN_22
#define GPIO_ADC_ACCEL_IOMUX_C8                                  (IOMUX_PINCM47)
#define GPIO_ADC_ACCEL_IOMUX_C8_FUNC              (IOMUX_PINCM47_PF_UNCONNECTED)
#define GPIO_ADC_ACCEL_C5_PORT                                             GPIOB
#define GPIO_ADC_ACCEL_C5_PIN                                     DL_GPIO_PIN_18
#define GPIO_ADC_ACCEL_IOMUX_C5                                  (IOMUX_PINCM44)
#define GPIO_ADC_ACCEL_IOMUX_C5_FUNC              (IOMUX_PINCM44_PF_UNCONNECTED)
#define GPIO_ADC_ACCEL_C0_PORT                                             GPIOA
#define GPIO_ADC_ACCEL_C0_PIN                                     DL_GPIO_PIN_15
#define GPIO_ADC_ACCEL_IOMUX_C0                                  (IOMUX_PINCM37)
#define GPIO_ADC_ACCEL_IOMUX_C0_FUNC              (IOMUX_PINCM37_PF_UNCONNECTED)



/* Port definition for Pin Group LCD_BACKLIGHT */
#define LCD_BACKLIGHT_PORT                                               (GPIOB)

/* Defines for PIN_0: GPIOB.1 with pinCMx 13 on package pin 48 */
#define LCD_BACKLIGHT_PIN_0_PIN                                  (DL_GPIO_PIN_1)
#define LCD_BACKLIGHT_PIN_0_IOMUX                                (IOMUX_PINCM13)
/* Port definition for Pin Group LCD_CS_PIN */
#define LCD_CS_PIN_PORT                                                  (GPIOA)

/* Defines for LCD_CS: GPIOA.2 with pinCMx 7 on package pin 42 */
#define LCD_CS_PIN_LCD_CS_PIN                                    (DL_GPIO_PIN_2)
#define LCD_CS_PIN_LCD_CS_IOMUX                                   (IOMUX_PINCM7)
/* Port definition for Pin Group GPIO_BUTTONS */
#define GPIO_BUTTONS_PORT                                                (GPIOA)

/* Defines for S1: GPIOA.11 with pinCMx 22 on package pin 57 */
// pins affected by this interrupt request:["S1","S2","JOY_SEL"]
#define GPIO_BUTTONS_INT_IRQN                                   (GPIOA_INT_IRQn)
#define GPIO_BUTTONS_INT_IIDX                   (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define GPIO_BUTTONS_S1_IIDX                                (DL_GPIO_IIDX_DIO11)
#define GPIO_BUTTONS_S1_PIN                                     (DL_GPIO_PIN_11)
#define GPIO_BUTTONS_S1_IOMUX                                    (IOMUX_PINCM22)
/* Defines for S2: GPIOA.18 with pinCMx 40 on package pin 11 */
#define GPIO_BUTTONS_S2_IIDX                                (DL_GPIO_IIDX_DIO18)
#define GPIO_BUTTONS_S2_PIN                                     (DL_GPIO_PIN_18)
#define GPIO_BUTTONS_S2_IOMUX                                    (IOMUX_PINCM40)
/* Defines for JOY_SEL: GPIOA.10 with pinCMx 21 on package pin 56 */
#define GPIO_BUTTONS_JOY_SEL_IIDX                           (DL_GPIO_IIDX_DIO10)
#define GPIO_BUTTONS_JOY_SEL_PIN                                (DL_GPIO_PIN_10)
#define GPIO_BUTTONS_JOY_SEL_IOMUX                               (IOMUX_PINCM21)
/* Port definition for Pin Group GPIO_RGB */
#define GPIO_RGB_PORT                                                    (GPIOB)

/* Defines for RED: GPIOB.22 with pinCMx 50 on package pin 21 */
#define GPIO_RGB_RED_PIN                                        (DL_GPIO_PIN_22)
#define GPIO_RGB_RED_IOMUX                                       (IOMUX_PINCM50)
/* Defines for GREEN: GPIOB.26 with pinCMx 57 on package pin 28 */
#define GPIO_RGB_GREEN_PIN                                      (DL_GPIO_PIN_26)
#define GPIO_RGB_GREEN_IOMUX                                     (IOMUX_PINCM57)
/* Defines for BLUE: GPIOB.27 with pinCMx 58 on package pin 29 */
#define GPIO_RGB_BLUE_PIN                                       (DL_GPIO_PIN_27)
#define GPIO_RGB_BLUE_IOMUX                                      (IOMUX_PINCM58)
/* Defines for RST: GPIOB.17 with pinCMx 43 on package pin 14 */
#define GPIO_LCD_RST_PORT                                                (GPIOB)
#define GPIO_LCD_RST_PIN                                        (DL_GPIO_PIN_17)
#define GPIO_LCD_RST_IOMUX                                       (IOMUX_PINCM43)
/* Defines for DC: GPIOA.13 with pinCMx 35 on package pin 6 */
#define GPIO_LCD_DC_PORT                                                 (GPIOA)
#define GPIO_LCD_DC_PIN                                         (DL_GPIO_PIN_13)
#define GPIO_LCD_DC_IOMUX                                        (IOMUX_PINCM35)





/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_SYSCTL_CLK_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_PWM_AUDIO_init(void);
void SYSCFG_DL_TIMER_SAMPLE_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_I2C_1_init(void);
void SYSCFG_DL_SPI_LCD_init(void);
void SYSCFG_DL_ADC_MIC_JOY_init(void);
void SYSCFG_DL_ADC_ACCEL_init(void);

void SYSCFG_DL_RTC_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
