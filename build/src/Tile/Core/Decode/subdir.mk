################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Core/Decode/DecodeStage.cpp \
../src/Tile/Core/Decode/Decoder.cpp \
../src/Tile/Core/Decode/ReceiveChannelEndTable.cpp 

OBJS += \
./src/Tile/Core/Decode/DecodeStage.o \
./src/Tile/Core/Decode/Decoder.o \
./src/Tile/Core/Decode/ReceiveChannelEndTable.o 

CPP_DEPS += \
./src/Tile/Core/Decode/DecodeStage.d \
./src/Tile/Core/Decode/Decoder.d \
./src/Tile/Core/Decode/ReceiveChannelEndTable.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Core/Decode/%.o: ../src/Tile/Core/Decode/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/local/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


