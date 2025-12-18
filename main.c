/**
 * @file main.c - NO LCD VERSION FOR TESTING
 * @brief MSPM0G3507 Synthesizer WITHOUT LCD
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "main.h"

SynthState_t g_synthState = {
    .waveform = WAVE_SINE,
    .mode = MODE_SYNTH,
    .frequency = FREQ_DEFAULT,
    .volume = VOLUME_DEFAULT,
    .audio_playing = false,
    .joy_x = JOY_ADC_CENTER,
    .joy_y = JOY_ADC_CENTER,
};

static const int16_t sine_table[256] = {
    0,50,100,150,200,249,297,344,391,436,481,524,565,606,644,681,716,749,
    780,809,836,861,883,903,921,936,949,959,967,972,975,975,973,968,961,
    951,939,924,907,887,865,841,814,785,754,721,686,649,611,571,529,486,
    442,397,350,303,255,206,157,108,58,9,-41,-90,-139,-187,-235,-282,-328,
    -373,-417,-460,-501,-541,-579,-616,-651,-684,-715,-744,-771,-796,-819,
    -839,-858,-874,-888,-900,-909,-916,-921,-923,-923,-920,-915,-908,-898,
    -886,-872,-855,-836,-815,-792,-766,-739,-709,-678,-645,-610,-573,-535,
    -495,-454,-412,-368,-323,-278,-231,-184,-136,-88,-39,9,58,107,155,203,
    250,296,341,385,428,470,510,549,586,622,656,688,718,747,773,798,820,
    841,859,875,889,901,911,918,923,926,927,925,922,916,908,898,886,871,
    855,836,816,793,769,743,715,686,655,622,588,552,515,477,438,397,356,
    313,270,226,182,137,91,46,0,-46,-91,-137,-182,-226,-270,-313,-356,-397,
    -438,-477,-515,-552,-588,-622,-655,-686,-715,-743,-769,-793,-816,-836,
    -855,-871,-886,-898,-908,-916,-922,-925,-927,-926,-923,-918,-911,-901,
    -889,-875,-859,-841,-820,-798,-773,-747,-718,-688,-656,-622,-586,-549,
    -510,-470,-428,-385,-341,-296,-250,-203,-155,-107,-58
};

static uint32_t phase = 0;
static uint32_t phase_increment = 0;

static void Update_Phase_Increment(void) {
    phase_increment = (uint32_t)((g_synthState.frequency * 4294967296.0) / SAMPLE_RATE);
}

static void Generate_Audio_Sample(void) {
    if (!g_synthState.audio_playing) {
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }

    uint8_t index = (uint8_t)((phase >> 24) & 0xFF);
    int16_t sample = sine_table[index];
    sample = (int16_t)((sample * g_synthState.volume) / 100);
    int32_t duty_temp = 2048 + sample;
    uint16_t duty = (uint16_t)CLAMP(duty_temp, 1, 4095);
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, duty, DL_TIMER_CC_0_INDEX);
    phase += phase_increment;
}

static void Process_Input(void) {
    static uint32_t last_update = 0;
    uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
    uint32_t elapsed = TIMER_ELAPSED(now, last_update);
    if (elapsed < (SYSCLK_FREQUENCY / SENSOR_UPDATE_HZ)) return;
    last_update = now;

    uint16_t joy_x_local = g_synthState.joy_x;
    uint16_t joy_y_local = g_synthState.joy_y;

    if (joy_x_local > (JOY_ADC_CENTER + JOY_DEADZONE) ||
        joy_x_local < (JOY_ADC_CENTER - JOY_DEADZONE)) {
        float ratio = (float)joy_x_local / (float)JOY_ADC_MAX;
        g_synthState.frequency = FREQ_MIN + (ratio * (FREQ_MAX - FREQ_MIN));
        Update_Phase_Increment();
    }

    if (joy_y_local > (JOY_ADC_CENTER + JOY_DEADZONE) ||
        joy_y_local < (JOY_ADC_CENTER - JOY_DEADZONE)) {
        g_synthState.volume = (uint8_t)((joy_y_local * 100UL) / JOY_ADC_MAX);
    }

    static bool last_s1 = false;
    bool btn_s1_local = g_synthState.btn_s1;
    if (btn_s1_local && !last_s1) {
        g_synthState.waveform = (Waveform_t)((g_synthState.waveform + 1) % WAVE_COUNT);
        g_synthState.btn_s1 = false;
        
        // RGB LED feedback
        DL_GPIO_clearPins(GPIO_RGB_RED_PORT, GPIO_RGB_RED_PIN);
        DL_GPIO_clearPins(GPIO_RGB_GREEN_PORT, GPIO_RGB_GREEN_PIN);
        DL_GPIO_clearPins(GPIO_RGB_BLUE_PORT, GPIO_RGB_BLUE_PIN);
        
        if (g_synthState.waveform == WAVE_SINE) {
            DL_GPIO_setPins(GPIO_RGB_GREEN_PORT, GPIO_RGB_GREEN_PIN);
        }
    }
    last_s1 = btn_s1_local;

    static bool last_s2 = false;
    bool btn_s2_local = g_synthState.btn_s2;
    if (btn_s2_local && !last_s2) {
        g_synthState.audio_playing = !g_synthState.audio_playing;
        g_synthState.btn_s2 = false;
        
        // Toggle RED LED as play indicator
        if (g_synthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_RED_PORT, GPIO_RGB_RED_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_RGB_RED_PORT, GPIO_RGB_RED_PIN);
        }
    }
    last_s2 = btn_s2_local;
}

void TIMG7_IRQHandler(void) {
    switch (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST)) {
        case DL_TIMER_IIDX_ZERO:
            Generate_Audio_Sample();
            break;
        default:
            break;
    }
}

void ADC0_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            g_synthState.mic_level = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
            break;
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            g_synthState.joy_y = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_1);
            break;
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            g_synthState.joy_x = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_2);
            break;
        default:
            break;
    }
}

void ADC1_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            g_synthState.accel_x = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
            break;
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            g_synthState.accel_y = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
            break;
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            g_synthState.accel_z = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
            break;
        default:
            break;
    }
}

void GPIOA_IRQHandler(void) {
    uint32_t status = DL_GPIO_getEnabledInterruptStatus(GPIOA,
        GPIO_BUTTONS_S1_PIN | GPIO_BUTTONS_S2_PIN | GPIO_BUTTONS_JOY_SEL_PIN);

    if (status & GPIO_BUTTONS_S1_PIN) {
        g_synthState.btn_s1 = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S1_PIN);
    }
    if (status & GPIO_BUTTONS_S2_PIN) {
        g_synthState.btn_s2 = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S2_PIN);
    }
    if (status & GPIO_BUTTONS_JOY_SEL_PIN) {
        g_synthState.joy_pressed = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_JOY_SEL_PIN);
    }
}

int main(void) {
    SYSCFG_DL_init();

    NVIC_EnableIRQ(TIMER_SAMPLE_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC_MIC_JOY_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC_ACCEL_INST_INT_IRQN);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    __enable_irq();

    // Initialize audio
    Update_Phase_Increment();
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);

    g_synthState.audio_playing = true;
    // Turn on GREEN LED to show ready
    DL_GPIO_setPins(GPIO_RGB_GREEN_PORT, GPIO_RGB_GREEN_PIN);

    while (1) {
        Process_Input();
    }
}

void delay_ms(uint32_t milliseconds) {
    uint64_t ticks = ((uint64_t)SYSCLK_FREQUENCY / 1000ULL) * milliseconds;
    if (ticks > TIMER_MAX_VALUE) ticks = TIMER_MAX_VALUE;
    uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
    uint32_t target_ticks = (uint32_t)ticks;
    while (TIMER_ELAPSED(DL_Timer_getTimerCount(TIMER_SAMPLE_INST), start) < target_ticks) {}
}

void delay_us(uint32_t microseconds) {
    uint64_t ticks = ((uint64_t)SYSCLK_FREQUENCY / 1000000ULL) * microseconds;
    if (ticks > TIMER_MAX_VALUE) ticks = TIMER_MAX_VALUE;
    uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
    uint32_t target_ticks = (uint32_t)ticks;
    while (TIMER_ELAPSED(DL_Timer_getTimerCount(TIMER_SAMPLE_INST), start) < target_ticks) {}
}