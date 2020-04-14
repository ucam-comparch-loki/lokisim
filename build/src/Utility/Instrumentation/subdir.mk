################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Utility/Instrumentation/ChannelMap.cpp \
../src/Utility/Instrumentation/FIFO.cpp \
../src/Utility/Instrumentation/IPKCache.cpp \
../src/Utility/Instrumentation/InstrumentationBase.cpp \
../src/Utility/Instrumentation/L1Cache.cpp \
../src/Utility/Instrumentation/Latency.cpp \
../src/Utility/Instrumentation/MainMemory.cpp \
../src/Utility/Instrumentation/Network.cpp \
../src/Utility/Instrumentation/Operations.cpp \
../src/Utility/Instrumentation/PipelineReg.cpp \
../src/Utility/Instrumentation/Registers.cpp \
../src/Utility/Instrumentation/Scratchpad.cpp \
../src/Utility/Instrumentation/Stalls.cpp 

OBJS += \
./src/Utility/Instrumentation/ChannelMap.o \
./src/Utility/Instrumentation/FIFO.o \
./src/Utility/Instrumentation/IPKCache.o \
./src/Utility/Instrumentation/InstrumentationBase.o \
./src/Utility/Instrumentation/L1Cache.o \
./src/Utility/Instrumentation/Latency.o \
./src/Utility/Instrumentation/MainMemory.o \
./src/Utility/Instrumentation/Network.o \
./src/Utility/Instrumentation/Operations.o \
./src/Utility/Instrumentation/PipelineReg.o \
./src/Utility/Instrumentation/Registers.o \
./src/Utility/Instrumentation/Scratchpad.o \
./src/Utility/Instrumentation/Stalls.o 

CPP_DEPS += \
./src/Utility/Instrumentation/ChannelMap.d \
./src/Utility/Instrumentation/FIFO.d \
./src/Utility/Instrumentation/IPKCache.d \
./src/Utility/Instrumentation/InstrumentationBase.d \
./src/Utility/Instrumentation/L1Cache.d \
./src/Utility/Instrumentation/Latency.d \
./src/Utility/Instrumentation/MainMemory.d \
./src/Utility/Instrumentation/Network.d \
./src/Utility/Instrumentation/Operations.d \
./src/Utility/Instrumentation/PipelineReg.d \
./src/Utility/Instrumentation/Registers.d \
./src/Utility/Instrumentation/Scratchpad.d \
./src/Utility/Instrumentation/Stalls.d 


# Each subdirectory must supply rules for building sources it contributes
src/Utility/Instrumentation/%.o: ../src/Utility/Instrumentation/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


