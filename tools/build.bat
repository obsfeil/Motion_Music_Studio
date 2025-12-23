@echo off
REM Build script for Motion Music Studio - Windows
REM Usage: build.bat [clean|debug|release]

setlocal

set PROJECT_NAME=Motion_Music_Studio
set CCS_PATH=C:\ti\ccs\ccs_base
set COMPILER=%CCS_PATH%\tools\compiler\ti-cgt-armllvm_4.0.4.LTS\bin\tiarmclang.exe
set SDK_PATH=C:\ti\mspm0_sdk_2_09_00_01

if "%1"=="clean" goto clean
if "%1"=="debug" goto debug
if "%1"=="release" goto release
goto debug

:clean
echo Cleaning project...
if exist Debug rmdir /s /q Debug
if exist Release rmdir /s /q Release
echo Clean complete!
goto end

:debug
echo Building DEBUG configuration...
echo.
echo Generating SysConfig files...
%CCS_PATH%\utils\sysconfig_cli\sysconfig_cli.bat ^
    -s "%SDK_PATH%\.metadata\product.json" ^
    --compiler ticlang ^
    --product %SDK_PATH% ^
    --output generated\ ^
    ti_msp_dl_config.syscfg

echo.
echo Compiling...
mkdir Debug 2>nul

"%COMPILER%" -c ^
    -march=thumbv6m ^
    -mcpu=cortex-m0plus ^
    -mfloat-abi=soft ^
    -mlittle-endian ^
    -mthumb ^
    -O0 ^
    -g ^
    -gdwarf-3 ^
    -I. ^
    -I"%SDK_PATH%\source" ^
    -I"%SDK_PATH%\source\third_party\CMSIS\Core\Include" ^
    -I"%SDK_PATH%\source\ti\iqmath\include" ^
    -DDEBUG_MODE=1 ^
    -DDEVICE_IS_MSPM0G3507 ^
    -o Debug\main.obj ^
    main.c

echo.
echo Linking...
"%COMPILER%" ^
    -march=thumbv6m ^
    -mcpu=cortex-m0plus ^
    -mfloat-abi=soft ^
    -Wl,-m,Debug\%PROJECT_NAME%.map ^
    -Wl,--rom_model ^
    -Wl,--warn_sections ^
    -L"%SDK_PATH%\source\ti\driverlib\lib\ticlang\m0p\mspm0g1x0x_g3x0x" ^
    -L"%SDK_PATH%\source\ti\iqmath\lib\ticlang\m0p\mathacl" ^
    -llibiqmath_MATHACL.a ^
    -ldriverlib.a ^
    -o Debug\%PROJECT_NAME%.out ^
    Debug\*.obj

echo.
echo Build complete: Debug\%PROJECT_NAME%.out
goto end

:release
echo Building RELEASE configuration...
echo (Implementation similar to debug, with -O2 and no -g)
goto end

:end
endlocal