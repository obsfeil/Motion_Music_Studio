# 🎯 Quick Build Instructions

## ⚡ Fast Track (5 minutes)

### 1. Create New Project in CCS
```
File → New → Project
→ Empty Project (No RTOS)
→ Device: MSPM0G3507
→ Name: Motion_Music_Studio_Fresh
```

### 2. Import Files
```
Copy all files from this package into project root:
- ti_msp_dl_config.syscfg
- main.c
- main.h
- (all other .c/.h files)
```

### 3. Build
```
Ctrl+B (Build)

Expected:
✅ 0 errors
⚠️ Maybe some warnings (OK!)
✅ .out file generated
```

### 4. Flash
```
F11 (Debug)
→ Wait for flash
F8 (Run)
→ Listen for tone!
```

## 🎵 What You'll Get

**Immediately working:**
- ✅ PWM audio output (12-bit)
- ✅ Joystick controls frequency
- ✅ Button S1 changes waveform
- ✅ Button S2 start/stop
- ✅ RGB LED indicators

**Phase 2 (add later):**
- LCD display
- Accelerometer pitch bend
- Microphone visualization
- Light sensor effects

## 🐛 If Build Fails

### Error: "ti_msp_dl_config.h not found"
```
1. Double-click ti_msp_dl_config.syscfg
2. Wait for SysConfig to open
3. Save (Ctrl+S)
4. Files will generate
5. Build again
```

### Error: "Cannot find -ldriverlib"
```
Right-click project → Properties
→ Build → ARM Linker → File Search Path
→ Add: ${COM_TI_MSPM0_SDK_INSTALL_DIR}/source
```

### Still Problems?
Send me screenshot of exact error!

