################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Utility/StartUp/CodeLoader.cpp \
../src/Utility/StartUp/DataBlock.cpp \
../src/Utility/StartUp/DataFileReader.cpp \
../src/Utility/StartUp/ELFFileReader.cpp \
../src/Utility/StartUp/FileReader.cpp \
../src/Utility/StartUp/LokiFileReader.cpp 

OBJS += \
./src/Utility/StartUp/CodeLoader.o \
./src/Utility/StartUp/DataBlock.o \
./src/Utility/StartUp/DataFileReader.o \
./src/Utility/StartUp/ELFFileReader.o \
./src/Utility/StartUp/FileReader.o \
./src/Utility/StartUp/LokiFileReader.o 

CPP_DEPS += \
./src/Utility/StartUp/CodeLoader.d \
./src/Utility/StartUp/DataBlock.d \
./src/Utility/StartUp/DataFileReader.d \
./src/Utility/StartUp/ELFFileReader.d \
./src/Utility/StartUp/FileReader.d \
./src/Utility/StartUp/LokiFileReader.d 


# Each subdirectory must supply rules for building sources it contributes
src/Utility/StartUp/%.o: ../src/Utility/StartUp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


