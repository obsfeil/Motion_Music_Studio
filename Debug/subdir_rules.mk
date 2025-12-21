################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
main.o: ../main.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -I"C:/ti/sysconfig_1.25.0" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -I"C:/ti/mspm0_sdk_2_08_00_04/source/ti/driverlib/lib/ticlang/m0p/mspm0g1x0x_g3x0x/driverlib.a" -I"C:/ti/mspm0_sdk_2_09_00_01/source/ti/driverlib/lib/ticlang/m0p/mspm0g1x0x_g3x0x" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -gdwarf-3 -fno-common -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(basename\ $(<F)).o"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1000533073: ../ti_msp_dl_config.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/sysconfig_1.25.0/sysconfig_cli.bat" -s "C:/ti/mspm0_sdk_2_09_00_01/.metadata/product.json" -b "/ti/boards/LP_MSPM0G3507" --script "C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/ti_msp_dl_config.syscfg" --context "system" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-1000533073 ../ti_msp_dl_config.syscfg
device.opt: build-1000533073
device.cmd.genlibs: build-1000533073
ti_msp_dl_config.c: build-1000533073
ti_msp_dl_config.h: build-1000533073
peripheralPinAssignments.txt: build-1000533073
resourceUsageReport.csv: build-1000533073
Event.dot: build-1000533073

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -I"C:/ti/sysconfig_1.25.0" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -gdwarf-3 -fno-common -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  @"./device.opt" -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: C:/ti/mspm0_sdk_2_09_00_01/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -I"C:/ti/sysconfig_1.25.0" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -gdwarf-3 -fno-common -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


