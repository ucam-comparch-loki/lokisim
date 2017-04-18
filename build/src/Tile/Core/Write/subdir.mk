################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Core/Write/SendChannelEndTable.cpp \
../src/Tile/Core/Write/WriteStage.cpp 

OBJS += \
./src/Tile/Core/Write/SendChannelEndTable.o \
./src/Tile/Core/Write/WriteStage.o 

CPP_DEPS += \
./src/Tile/Core/Write/SendChannelEndTable.d \
./src/Tile/Core/Write/WriteStage.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Core/Write/%.o: ../src/Tile/Core/Write/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/local/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


