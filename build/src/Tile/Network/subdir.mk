################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Network/CoreMulticast.cpp \
../src/Tile/Network/DataReturn.cpp \
../src/Tile/Network/ForwardCrossbar.cpp \
../src/Tile/Network/InstructionReturn.cpp 

OBJS += \
./src/Tile/Network/CoreMulticast.o \
./src/Tile/Network/DataReturn.o \
./src/Tile/Network/ForwardCrossbar.o \
./src/Tile/Network/InstructionReturn.o 

CPP_DEPS += \
./src/Tile/Network/CoreMulticast.d \
./src/Tile/Network/DataReturn.d \
./src/Tile/Network/ForwardCrossbar.d \
./src/Tile/Network/InstructionReturn.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Network/%.o: ../src/Tile/Network/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/local/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

