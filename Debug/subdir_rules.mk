################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_09_00_01/source/ti/iqmath/include" -I"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/include" -D__MSPM0G3507__ -DDEVICE_IS_MSPM0G3507 -gdwarf-3 -fno-common -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug/syscfg"  @"syscfg/device.opt" -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1000533073: ../ti_msp_dl_config.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/sysconfig_1.25.0/sysconfig_cli.bat" -s "C:/ti/mspm0_sdk_2_09_00_01/.metadata/product.json" -b "/ti/boards/LP_MSPM0G3507" --script "C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/ti_msp_dl_config.syscfg" -o "syscfg" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/device_linker.cmd: build-1000533073 ../ti_msp_dl_config.syscfg
syscfg/device.opt: build-1000533073
syscfg/device.cmd.genlibs: build-1000533073
syscfg/ti_msp_dl_config.c: build-1000533073
syscfg/ti_msp_dl_config.h: build-1000533073
syscfg/peripheralPinAssignments.txt: build-1000533073
syscfg/resourceUsageReport.csv: build-1000533073
syscfg/Event.dot: build-1000533073
syscfg: build-1000533073

syscfg/%.o: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_09_00_01/source/ti/iqmath/include" -I"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/include" -D__MSPM0G3507__ -DDEVICE_IS_MSPM0G3507 -gdwarf-3 -fno-common -MMD -MP -MF"syscfg/$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug/syscfg"  @"syscfg/device.opt" -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: C:/ti/mspm0_sdk_2_09_00_01/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_09_00_01/source/ti/iqmath/include" -I"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/include" -D__MSPM0G3507__ -DDEVICE_IS_MSPM0G3507 -gdwarf-3 -fno-common -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug/syscfg"  @"syscfg/device.opt" -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


