################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Memory/MemoryOperations/AtomicOperations.cpp \
../src/Memory/MemoryOperations/BasicOperations.cpp \
../src/Memory/MemoryOperations/CacheLineOperations.cpp \
../src/Memory/MemoryOperations/DirectoryOperations.cpp \
../src/Memory/MemoryOperations/MemoryOperation.cpp 

OBJS += \
./src/Memory/MemoryOperations/AtomicOperations.o \
./src/Memory/MemoryOperations/BasicOperations.o \
./src/Memory/MemoryOperations/CacheLineOperations.o \
./src/Memory/MemoryOperations/DirectoryOperations.o \
./src/Memory/MemoryOperations/MemoryOperation.o 

CPP_DEPS += \
./src/Memory/MemoryOperations/AtomicOperations.d \
./src/Memory/MemoryOperations/BasicOperations.d \
./src/Memory/MemoryOperations/CacheLineOperations.d \
./src/Memory/MemoryOperations/DirectoryOperations.d \
./src/Memory/MemoryOperations/MemoryOperation.d 


# Each subdirectory must supply rules for building sources it contributes
src/Memory/MemoryOperations/%.o: ../src/Memory/MemoryOperations/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


