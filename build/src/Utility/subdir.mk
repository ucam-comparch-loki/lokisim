################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Utility/Arguments.cpp \
../src/Utility/BlockingInterface.cpp \
../src/Utility/Config.cpp \
../src/Utility/Debugger.cpp \
../src/Utility/ISA.cpp \
../src/Utility/Instrumentation.cpp \
../src/Utility/Parameters.cpp \
../src/Utility/Statistics.cpp \
../src/Utility/StringManipulation.cpp \
../src/Utility/Warnings.cpp 

OBJS += \
./src/Utility/Arguments.o \
./src/Utility/BlockingInterface.o \
./src/Utility/Config.o \
./src/Utility/Debugger.o \
./src/Utility/ISA.o \
./src/Utility/Instrumentation.o \
./src/Utility/Parameters.o \
./src/Utility/Statistics.o \
./src/Utility/StringManipulation.o \
./src/Utility/Warnings.o 

CPP_DEPS += \
./src/Utility/Arguments.d \
./src/Utility/BlockingInterface.d \
./src/Utility/Config.d \
./src/Utility/Debugger.d \
./src/Utility/ISA.d \
./src/Utility/Instrumentation.d \
./src/Utility/Parameters.d \
./src/Utility/Statistics.d \
./src/Utility/StringManipulation.d \
./src/Utility/Warnings.d 


# Each subdirectory must supply rules for building sources it contributes
src/Utility/%.o: ../src/Utility/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/local/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


