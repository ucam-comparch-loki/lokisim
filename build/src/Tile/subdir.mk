################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/ChannelMapEntry.cpp \
../src/Tile/ComputeTile.cpp \
../src/Tile/EmptyTile.cpp \
../src/Tile/MemoryControllerTile.cpp \
../src/Tile/Tile.cpp 

OBJS += \
./src/Tile/ChannelMapEntry.o \
./src/Tile/ComputeTile.o \
./src/Tile/EmptyTile.o \
./src/Tile/MemoryControllerTile.o \
./src/Tile/Tile.o 

CPP_DEPS += \
./src/Tile/ChannelMapEntry.d \
./src/Tile/ComputeTile.d \
./src/Tile/EmptyTile.d \
./src/Tile/MemoryControllerTile.d \
./src/Tile/Tile.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/%.o: ../src/Tile/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


