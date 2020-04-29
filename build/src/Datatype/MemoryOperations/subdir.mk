################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Datatype/MemoryOperations/AtomicOperations.cpp \
../src/Datatype/MemoryOperations/BasicOperations.cpp \
../src/Datatype/MemoryOperations/CacheLineOperations.cpp \
../src/Datatype/MemoryOperations/DirectoryOperations.cpp \
../src/Datatype/MemoryOperations/MemoryOperation.cpp 

OBJS += \
./src/Datatype/MemoryOperations/AtomicOperations.o \
./src/Datatype/MemoryOperations/BasicOperations.o \
./src/Datatype/MemoryOperations/CacheLineOperations.o \
./src/Datatype/MemoryOperations/DirectoryOperations.o \
./src/Datatype/MemoryOperations/MemoryOperation.o 

CPP_DEPS += \
./src/Datatype/MemoryOperations/AtomicOperations.d \
./src/Datatype/MemoryOperations/BasicOperations.d \
./src/Datatype/MemoryOperations/CacheLineOperations.d \
./src/Datatype/MemoryOperations/DirectoryOperations.d \
./src/Datatype/MemoryOperations/MemoryOperation.d 


# Each subdirectory must supply rules for building sources it contributes
src/Datatype/MemoryOperations/%.o: ../src/Datatype/MemoryOperations/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


