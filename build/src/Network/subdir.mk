################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/Multiplexer.cpp \
../src/Network/Network.cpp \
../src/Network/Router.cpp 

OBJS += \
./src/Network/Multiplexer.o \
./src/Network/Network.o \
./src/Network/Router.o 

CPP_DEPS += \
./src/Network/Multiplexer.d \
./src/Network/Network.d \
./src/Network/Router.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/%.o: ../src/Network/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/local/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

