################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
ti/driverlib/m0p/sysctl/%.o: ../ti/driverlib/m0p/sysctl/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio" -I"C:/Users/obsfe/workspace_ccstheia/Motion_Music_studio/Debug" -I"C:/ti/mspm0_sdk_2_08_00_04/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_08_00_04/source" -gdwarf-3 -MMD -MP -MF"ti/driverlib/m0p/sysctl/$(basename $(<F)).d_raw" -MT"$(@)"  @"./device.opt" -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


