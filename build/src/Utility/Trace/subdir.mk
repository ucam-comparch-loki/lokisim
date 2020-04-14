################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Utility/Trace/Callgrind.cpp 

OBJS += \
./src/Utility/Trace/Callgrind.o 

CPP_DEPS += \
./src/Utility/Trace/Callgrind.d 


# Each subdirectory must supply rules for building sources it contributes
src/Utility/Trace/%.o: ../src/Utility/Trace/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


