static void Process_Joystick(void) {
    // Dead zone: Only respond to significant joystick movements
    #define JOY_DEAD_ZONE 50
    #define JOY_HYSTERESIS 20  // Prevents oscillation
    
    // Joystick X controls frequency
    if (gSynthState.joy_x > (2048 + JOY_DEAD_ZONE) || 
        gSynthState.joy_x < (2048 - JOY_DEAD_ZONE)) {
        
        uint32_t freq_int = FREQ_MIN_HZ + ((gSynthState.joy_x * (FREQ_MAX_HZ - FREQ_MIN_HZ)) / 4095);

        freq_int = Smooth_Frequency(freq_int);
        
        // Hysteresis: Only update if difference is significant
        uint32_t diff = (freq_int > base_frequency_hz) ? 
                        (freq_int - base_frequency_hz) : 
                        (base_frequency_hz - freq_int);
        
        if (diff > JOY_HYSTERESIS) {  // Changed from 10 to 20
            base_frequency_hz = freq_int;
            Update_Phase_Increment();
        }
    }
    
    // Joystick Y controls volume
    if (gSynthState.joy_y > (2048 + JOY_DEAD_ZONE) || 
        gSynthState.joy_y < (2048 - JOY_DEAD_ZONE)) {
        
        uint8_t new_vol = (uint8_t)((gSynthState.joy_y * 100) / 4095);
        if (new_vol > 100) new_vol = 100;
        
        // Only update if change is significant
        uint8_t vol_diff = (new_vol > gSynthState.volume) ?
                          (new_vol - gSynthState.volume) :
                          (gSynthState.volume - new_vol);
        
        if (vol_diff > 2) {  // Ignore tiny volume changes
            gSynthState.volume = new_vol;
        }
    }
}