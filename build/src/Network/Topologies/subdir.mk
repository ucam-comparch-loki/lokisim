################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/Topologies/Bus.cpp \
../src/Network/Topologies/Crossbar.cpp \
../src/Network/Topologies/InstantCrossbar.cpp \
../src/Network/Topologies/Mesh.cpp \
../src/Network/Topologies/MulticastBus.cpp \
../src/Network/Topologies/MulticastNetwork.cpp 

OBJS += \
./src/Network/Topologies/Bus.o \
./src/Network/Topologies/Crossbar.o \
./src/Network/Topologies/InstantCrossbar.o \
./src/Network/Topologies/Mesh.o \
./src/Network/Topologies/MulticastBus.o \
./src/Network/Topologies/MulticastNetwork.o 

CPP_DEPS += \
./src/Network/Topologies/Bus.d \
./src/Network/Topologies/Crossbar.d \
./src/Network/Topologies/InstantCrossbar.d \
./src/Network/Topologies/Mesh.d \
./src/Network/Topologies/MulticastBus.d \
./src/Network/Topologies/MulticastNetwork.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/Topologies/%.o: ../src/Network/Topologies/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/usr/local/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


