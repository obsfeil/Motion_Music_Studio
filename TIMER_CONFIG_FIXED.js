/**
 * Motion Music Studio - Corrected Timer Configuration
 * 
 * TIMER0 (TIMG0): PWM Audio Output @ 19.5 kHz
 * TIMER7 (TIMG7): Sample Rate Generator @ 20 kHz
 */

/**
 * TIMER0 - PWM for Audio Output
 * 
 * PWM Frequency: 19.5 kHz
 * Period = 1 / 19,500 Hz = 51.28 us
 * Resolution: 12-bit (4096 levels)
 * Load Value = (80 MHz / 19,500 Hz) - 1 = 4,102
 */
TIMER1.$name                       = "PWM_AUDIO";
TIMER1.timerMode                   = "PERIODIC";
TIMER1.timerClkSrc                 = "BUSCLK";  // 80 MHz
TIMER1.timerClkPrescale            = 1;         // No prescaler
TIMER1.timerPeriod                 = "51.28 us";  // ✅ CORRECTED: 1/19500 Hz
TIMER1.timerStartTimer             = true;

// PWM Configuration
TIMER1.pwmMode                     = "EDGE_ALIGN";
TIMER1.ccIndex                     = [0];
TIMER1.captureCompareValue0        = 2047;  // 50% duty = center/silence

// Pin assignment (check your schematic!)
TIMER1.peripheral.$assign          = "TIMG0";
TIMER1.peripheral.ccp0Pin.$assign  = "PB4";   // Verify this pin!

/**
 * TIMER7 - Audio Sample Rate Generator
 * 
 * Sample Rate: 20 kHz
 * Period = 1 / 20,000 Hz = 50 us
 * Load Value = (80 MHz / 20,000 Hz) - 1 = 3,999
 */
TIMER2.$name                       = "TIMER_SAMPLE";
TIMER2.timerMode                   = "PERIODIC";
TIMER2.timerClkSrc                 = "BUSCLK";  // 80 MHz
TIMER2.timerClkPrescale            = 1;         // No prescaler
TIMER2.timerPeriod                 = "50 us";   // ✅ Already correct: 1/20000 Hz
TIMER2.timerStartTimer             = true;

// Interrupt configuration
TIMER2.interrupts                  = ["ZERO"];
TIMER2.interruptPriority           = "1";  // High priority for audio

// No pin needed (internal timer only)
TIMER2.peripheral.$assign          = "TIMG7";
