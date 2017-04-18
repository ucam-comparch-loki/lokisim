################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/Arbiters/ClockedArbiter.cpp 

OBJS += \
./src/Network/Arbiters/ClockedArbiter.o 

CPP_DEPS += \
./src/Network/Arbiters/ClockedArbiter.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/Arbiters/%.o: ../src/Network/Arbiters/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


