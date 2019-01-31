################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Core/Execute/ALU.cpp \
../src/Tile/Core/Execute/ExecuteStage.cpp \
../src/Tile/Core/Execute/Scratchpad.cpp 

OBJS += \
./src/Tile/Core/Execute/ALU.o \
./src/Tile/Core/Execute/ExecuteStage.o \
./src/Tile/Core/Execute/Scratchpad.o 

CPP_DEPS += \
./src/Tile/Core/Execute/ALU.d \
./src/Tile/Core/Execute/ExecuteStage.d \
./src/Tile/Core/Execute/Scratchpad.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Core/Execute/%.o: ../src/Tile/Core/Execute/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


