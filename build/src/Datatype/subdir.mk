################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Datatype/DecodedInst.cpp \
../src/Datatype/Encoding.cpp \
../src/Datatype/Flit.cpp \
../src/Datatype/Instruction.cpp 

OBJS += \
./src/Datatype/DecodedInst.o \
./src/Datatype/Encoding.o \
./src/Datatype/Flit.o \
./src/Datatype/Instruction.o 

CPP_DEPS += \
./src/Datatype/DecodedInst.d \
./src/Datatype/Encoding.d \
./src/Datatype/Flit.d \
./src/Datatype/Instruction.d 


# Each subdirectory must supply rules for building sources it contributes
src/Datatype/%.o: ../src/Datatype/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


