################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Tile/Network/BankToMHLRequests.cpp \
../src/Tile/Network/CoreMulticast.cpp \
../src/Tile/Network/CreditReturn.cpp \
../src/Tile/Network/DataReturn.cpp \
../src/Tile/Network/ForwardCrossbar.cpp \
../src/Tile/Network/InstructionReturn.cpp \
../src/Tile/Network/IntertileUnit.cpp \
../src/Tile/Network/MHLToBankResponses.cpp 

OBJS += \
./src/Tile/Network/BankToMHLRequests.o \
./src/Tile/Network/CoreMulticast.o \
./src/Tile/Network/CreditReturn.o \
./src/Tile/Network/DataReturn.o \
./src/Tile/Network/ForwardCrossbar.o \
./src/Tile/Network/InstructionReturn.o \
./src/Tile/Network/IntertileUnit.o \
./src/Tile/Network/MHLToBankResponses.o 

CPP_DEPS += \
./src/Tile/Network/BankToMHLRequests.d \
./src/Tile/Network/CoreMulticast.d \
./src/Tile/Network/CreditReturn.d \
./src/Tile/Network/DataReturn.d \
./src/Tile/Network/ForwardCrossbar.d \
./src/Tile/Network/InstructionReturn.d \
./src/Tile/Network/IntertileUnit.d \
./src/Tile/Network/MHLToBankResponses.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tile/Network/%.o: ../src/Tile/Network/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


