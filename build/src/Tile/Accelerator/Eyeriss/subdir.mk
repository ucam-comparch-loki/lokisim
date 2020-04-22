################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Accelerator/Eyeriss/Configuration.cpp \
../src/Tile/Accelerator/Eyeriss/Interconnect.cpp \
../src/Tile/Accelerator/Eyeriss/Mapping.cpp \
../src/Tile/Accelerator/Eyeriss/MulticastController.cpp \
../src/Tile/Accelerator/Eyeriss/PE.cpp 

OBJS += \
./src/Tile/Accelerator/Eyeriss/Configuration.o \
./src/Tile/Accelerator/Eyeriss/Interconnect.o \
./src/Tile/Accelerator/Eyeriss/Mapping.o \
./src/Tile/Accelerator/Eyeriss/MulticastController.o \
./src/Tile/Accelerator/Eyeriss/PE.o 

CPP_DEPS += \
./src/Tile/Accelerator/Eyeriss/Configuration.d \
./src/Tile/Accelerator/Eyeriss/Interconnect.d \
./src/Tile/Accelerator/Eyeriss/Mapping.d \
./src/Tile/Accelerator/Eyeriss/MulticastController.d \
./src/Tile/Accelerator/Eyeriss/PE.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Accelerator/Eyeriss/%.o: ../src/Tile/Accelerator/Eyeriss/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


