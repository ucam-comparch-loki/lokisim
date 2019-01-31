################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Memory/IPKCacheBase.cpp \
../src/Memory/IPKCacheDirectMapped.cpp \
../src/Memory/IPKCacheFullyAssociative.cpp 

OBJS += \
./src/Memory/IPKCacheBase.o \
./src/Memory/IPKCacheDirectMapped.o \
./src/Memory/IPKCacheFullyAssociative.o 

CPP_DEPS += \
./src/Memory/IPKCacheBase.d \
./src/Memory/IPKCacheDirectMapped.d \
./src/Memory/IPKCacheFullyAssociative.d 


# Each subdirectory must supply rules for building sources it contributes
src/Memory/%.o: ../src/Memory/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I/usr/groups/comparch-loki/tools/releases/systemc/current/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


