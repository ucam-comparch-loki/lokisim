################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Core/Fetch/FetchStage.cpp \
../src/Tile/Core/Fetch/InstructionPacketCache.cpp \
../src/Tile/Core/Fetch/InstructionPacketFIFO.cpp 

OBJS += \
./src/Tile/Core/Fetch/FetchStage.o \
./src/Tile/Core/Fetch/InstructionPacketCache.o \
./src/Tile/Core/Fetch/InstructionPacketFIFO.o 

CPP_DEPS += \
./src/Tile/Core/Fetch/FetchStage.d \
./src/Tile/Core/Fetch/InstructionPacketCache.d \
./src/Tile/Core/Fetch/InstructionPacketFIFO.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Core/Fetch/%.o: ../src/Tile/Core/Fetch/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


