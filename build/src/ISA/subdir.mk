################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ISA/ISA.cpp \
../src/ISA/InstructionBase.cpp \
../src/ISA/InstructionDecode.cpp \
../src/ISA/InstructionInterface.cpp 

OBJS += \
./src/ISA/ISA.o \
./src/ISA/InstructionBase.o \
./src/ISA/InstructionDecode.o \
./src/ISA/InstructionInterface.o 

CPP_DEPS += \
./src/ISA/ISA.d \
./src/ISA/InstructionBase.d \
./src/ISA/InstructionDecode.d \
./src/ISA/InstructionInterface.d 


# Each subdirectory must supply rules for building sources it contributes
src/ISA/%.o: ../src/ISA/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

