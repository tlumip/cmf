################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ctlutil.cpp \
../src/stdutil.cpp \
../src/wlklinks.cpp 

OBJS += \
./src/ctlutil.o \
./src/stdutil.o \
./src/wlklinks.o 

CPP_DEPS += \
./src/ctlutil.d \
./src/stdutil.d \
./src/wlklinks.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


