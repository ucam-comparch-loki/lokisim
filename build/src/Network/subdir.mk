################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/Network.cpp \
../src/Network/Router.cpp 

OBJS += \
./src/Network/Network.o \
./src/Network/Router.o 

CPP_DEPS += \
./src/Network/Network.d \
./src/Network/Router.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/%.o: ../src/Network/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


