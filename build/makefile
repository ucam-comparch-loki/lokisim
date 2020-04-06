################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm

# All of the sources participating in the build are defined here
-include sources.mk
-include src/Utility/Trace/subdir.mk
-include src/Utility/StartUp/subdir.mk
-include src/Utility/Instrumentation/subdir.mk
-include src/Utility/subdir.mk
-include src/Tile/Network/subdir.mk
-include src/Tile/Memory/subdir.mk
-include src/Tile/Core/Write/subdir.mk
-include src/Tile/Core/Fetch/subdir.mk
-include src/Tile/Core/Execute/subdir.mk
-include src/Tile/Core/Decode/subdir.mk
-include src/Tile/Core/subdir.mk
-include src/Tile/subdir.mk
-include src/OffChip/subdir.mk
-include src/Network/Topologies/subdir.mk
-include src/Network/Global/subdir.mk
-include src/Network/subdir.mk
-include src/Memory/subdir.mk
-include src/Datatype/MemoryOperations/subdir.mk
-include src/Datatype/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lokisim

# Tool invocations
lokisim: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "lokisim" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) lokisim
	-@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
