################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Memory/Directory.cpp \
../src/Tile/Memory/L2RequestFilter.cpp \
../src/Tile/Memory/MemoryBank.cpp \
../src/Tile/Memory/MissHandlingLogic.cpp \
../src/Tile/Memory/ReservationHandler.cpp 

OBJS += \
./src/Tile/Memory/Directory.o \
./src/Tile/Memory/L2RequestFilter.o \
./src/Tile/Memory/MemoryBank.o \
./src/Tile/Memory/MissHandlingLogic.o \
./src/Tile/Memory/ReservationHandler.o 

CPP_DEPS += \
./src/Tile/Memory/Directory.d \
./src/Tile/Memory/L2RequestFilter.d \
./src/Tile/Memory/MemoryBank.d \
./src/Tile/Memory/MissHandlingLogic.d \
./src/Tile/Memory/ReservationHandler.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Memory/%.o: ../src/Tile/Memory/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


