################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Arbitration/ArbiterBase.cpp \
../src/Arbitration/RoundRobinArbiter.cpp 

OBJS += \
./src/Arbitration/ArbiterBase.o \
./src/Arbitration/RoundRobinArbiter.o 

CPP_DEPS += \
./src/Arbitration/ArbiterBase.d \
./src/Arbitration/RoundRobinArbiter.d 


# Each subdirectory must supply rules for building sources it contributes
src/Arbitration/%.o: ../src/Arbitration/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


