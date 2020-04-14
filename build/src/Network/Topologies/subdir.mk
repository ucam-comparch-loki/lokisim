################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Network/Topologies/Mesh.cpp 

OBJS += \
./src/Network/Topologies/Mesh.o 

CPP_DEPS += \
./src/Network/Topologies/Mesh.d 


# Each subdirectory must supply rules for building sources it contributes
src/Network/Topologies/%.o: ../src/Network/Topologies/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -I$(SYSTEMC_DIR)/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


