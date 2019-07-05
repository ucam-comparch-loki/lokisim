################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Accelerator/Accelerator.cpp \
../src/Tile/Accelerator/CommandQueue.cpp \
../src/Tile/Accelerator/ComputeStage.cpp \
../src/Tile/Accelerator/ComputeUnit.cpp \
../src/Tile/Accelerator/ControlUnit.cpp \
../src/Tile/Accelerator/ConvolutionAlgorithm.cpp \
../src/Tile/Accelerator/DMA.cpp \
../src/Tile/Accelerator/Loops.cpp \
../src/Tile/Accelerator/MemoryInterface.cpp \
../src/Tile/Accelerator/ParameterReceiver.cpp 

OBJS += \
./src/Tile/Accelerator/Accelerator.o \
./src/Tile/Accelerator/CommandQueue.o \
./src/Tile/Accelerator/ComputeStage.o \
./src/Tile/Accelerator/ComputeUnit.o \
./src/Tile/Accelerator/ControlUnit.o \
./src/Tile/Accelerator/ConvolutionAlgorithm.o \
./src/Tile/Accelerator/DMA.o \
./src/Tile/Accelerator/Loops.o \
./src/Tile/Accelerator/MemoryInterface.o \
./src/Tile/Accelerator/ParameterReceiver.o 

CPP_DEPS += \
./src/Tile/Accelerator/Accelerator.d \
./src/Tile/Accelerator/CommandQueue.d \
./src/Tile/Accelerator/ComputeStage.d \
./src/Tile/Accelerator/ComputeUnit.d \
./src/Tile/Accelerator/ControlUnit.d \
./src/Tile/Accelerator/ConvolutionAlgorithm.d \
./src/Tile/Accelerator/DMA.d \
./src/Tile/Accelerator/Loops.d \
./src/Tile/Accelerator/MemoryInterface.d \
./src/Tile/Accelerator/ParameterReceiver.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Accelerator/%.o: ../src/Tile/Accelerator/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


