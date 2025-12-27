################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
SYSCFG_SRCS += \
../ti_msp_dl_config.syscfg 

C_SRCS += \
../buttons.c \
../lcd_driver.c \
../main.c \
./syscfg/ti_msp_dl_config.c \
C:/ti/mspm0_sdk_2_09_00_01/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c 

GEN_CMDS += \
./syscfg/device_linker.cmd 

GEN_FILES += \
./syscfg/device_linker.cmd \
./syscfg/device.opt \
./syscfg/ti_msp_dl_config.c 

GEN_MISC_DIRS += \
./syscfg 

C_DEPS += \
./buttons.d \
./lcd_driver.d \
./main.d \
./syscfg/ti_msp_dl_config.d \
./startup_mspm0g350x_ticlang.d 

GEN_OPTS += \
./syscfg/device.opt 

OBJS += \
./buttons.o \
./lcd_driver.o \
./main.o \
./syscfg/ti_msp_dl_config.o \
./startup_mspm0g350x_ticlang.o 

GEN_MISC_FILES += \
./syscfg/device.cmd.genlibs \
./syscfg/ti_msp_dl_config.h \
./syscfg/peripheralPinAssignments.txt \
./syscfg/resourceUsageReport.csv \
./syscfg/Event.dot 

GEN_MISC_DIRS__QUOTED += \
"syscfg" 

OBJS__QUOTED += \
"buttons.o" \
"lcd_driver.o" \
"main.o" \
"syscfg\ti_msp_dl_config.o" \
"startup_mspm0g350x_ticlang.o" 

GEN_MISC_FILES__QUOTED += \
"syscfg\device.cmd.genlibs" \
"syscfg\ti_msp_dl_config.h" \
"syscfg\peripheralPinAssignments.txt" \
"syscfg\resourceUsageReport.csv" \
"syscfg\Event.dot" 

C_DEPS__QUOTED += \
"buttons.d" \
"lcd_driver.d" \
"main.d" \
"syscfg\ti_msp_dl_config.d" \
"startup_mspm0g350x_ticlang.d" 

GEN_FILES__QUOTED += \
"syscfg\device_linker.cmd" \
"syscfg\device.opt" \
"syscfg\ti_msp_dl_config.c" 

C_SRCS__QUOTED += \
"../buttons.c" \
"../lcd_driver.c" \
"./syscfg/ti_msp_dl_config.c" \
"C:/ti/mspm0_sdk_2_09_00_01/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c" 

SYSCFG_SRCS__QUOTED += \
"../ti_msp_dl_config.syscfg" 


