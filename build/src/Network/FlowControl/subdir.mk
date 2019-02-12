################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/FlowControl/FlowControlIn.cpp \
../src/Network/FlowControl/FlowControlOut.cpp 

OBJS += \
./src/Network/FlowControl/FlowControlIn.o \
./src/Network/FlowControl/FlowControlOut.o 

CPP_DEPS += \
./src/Network/FlowControl/FlowControlIn.d \
./src/Network/FlowControl/FlowControlOut.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/FlowControl/%.o: ../src/Network/FlowControl/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


