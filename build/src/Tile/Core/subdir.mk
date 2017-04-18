################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Core/ChannelMapTable.cpp \
../src/Tile/Core/ControlRegisters.cpp \
../src/Tile/Core/Core.cpp \
../src/Tile/Core/InputCrossbar.cpp \
../src/Tile/Core/MagicMemoryConnection.cpp \
../src/Tile/Core/PipelineRegister.cpp \
../src/Tile/Core/PipelineStage.cpp \
../src/Tile/Core/PredicateRegister.cpp \
../src/Tile/Core/RegisterFile.cpp 

OBJS += \
./src/Tile/Core/ChannelMapTable.o \
./src/Tile/Core/ControlRegisters.o \
./src/Tile/Core/Core.o \
./src/Tile/Core/InputCrossbar.o \
./src/Tile/Core/MagicMemoryConnection.o \
./src/Tile/Core/PipelineRegister.o \
./src/Tile/Core/PipelineStage.o \
./src/Tile/Core/PredicateRegister.o \
./src/Tile/Core/RegisterFile.o 

CPP_DEPS += \
./src/Tile/Core/ChannelMapTable.d \
./src/Tile/Core/ControlRegisters.d \
./src/Tile/Core/Core.d \
./src/Tile/Core/InputCrossbar.d \
./src/Tile/Core/MagicMemoryConnection.d \
./src/Tile/Core/PipelineRegister.d \
./src/Tile/Core/PipelineStage.d \
./src/Tile/Core/PredicateRegister.d \
./src/Tile/Core/RegisterFile.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Core/%.o: ../src/Tile/Core/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


