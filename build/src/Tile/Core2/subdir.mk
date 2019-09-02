################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Core2/Core.cpp \
../src/Tile/Core2/DecodeStage.cpp \
../src/Tile/Core2/ExecuteStage.cpp \
../src/Tile/Core2/FetchStage.cpp \
../src/Tile/Core2/InputFIFOs.cpp \
../src/Tile/Core2/InstructionHandler.cpp \
../src/Tile/Core2/PipelineStage.cpp \
../src/Tile/Core2/Storage.cpp \
../src/Tile/Core2/SystemCall.cpp \
../src/Tile/Core2/WriteStage.cpp 

OBJS += \
./src/Tile/Core2/Core.o \
./src/Tile/Core2/DecodeStage.o \
./src/Tile/Core2/ExecuteStage.o \
./src/Tile/Core2/FetchStage.o \
./src/Tile/Core2/InputFIFOs.o \
./src/Tile/Core2/InstructionHandler.o \
./src/Tile/Core2/PipelineStage.o \
./src/Tile/Core2/Storage.o \
./src/Tile/Core2/SystemCall.o \
./src/Tile/Core2/WriteStage.o 

CPP_DEPS += \
./src/Tile/Core2/Core.d \
./src/Tile/Core2/DecodeStage.d \
./src/Tile/Core2/ExecuteStage.d \
./src/Tile/Core2/FetchStage.d \
./src/Tile/Core2/InputFIFOs.d \
./src/Tile/Core2/InstructionHandler.d \
./src/Tile/Core2/PipelineStage.d \
./src/Tile/Core2/Storage.d \
./src/Tile/Core2/SystemCall.d \
./src/Tile/Core2/WriteStage.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Core2/%.o: ../src/Tile/Core2/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


