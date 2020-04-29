################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/Global/CreditNetwork.cpp \
../src/Network/Global/DataNetwork.cpp \
../src/Network/Global/RequestNetwork.cpp \
../src/Network/Global/ResponseNetwork.cpp 

OBJS += \
./src/Network/Global/CreditNetwork.o \
./src/Network/Global/DataNetwork.o \
./src/Network/Global/RequestNetwork.o \
./src/Network/Global/ResponseNetwork.o 

CPP_DEPS += \
./src/Network/Global/CreditNetwork.d \
./src/Network/Global/DataNetwork.d \
./src/Network/Global/RequestNetwork.d \
./src/Network/Global/ResponseNetwork.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/Global/%.o: ../src/Network/Global/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


