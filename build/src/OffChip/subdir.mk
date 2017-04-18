################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/OffChip/MagicMemory.cpp \
../src/OffChip/MainMemory.cpp \
../src/OffChip/MainMemoryRequestHandler.cpp 

OBJS += \
./src/OffChip/MagicMemory.o \
./src/OffChip/MainMemory.o \
./src/OffChip/MainMemoryRequestHandler.o 

CPP_DEPS += \
./src/OffChip/MagicMemory.d \
./src/OffChip/MainMemory.d \
./src/OffChip/MainMemoryRequestHandler.d 


# Each subdirectory must supply rules for building sources it contributes
src/OffChip/%.o: ../src/OffChip/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


