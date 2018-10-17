################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/INIReader.cpp \
../src/aac.cpp \
../src/accordion_V9.x.cpp \
../src/base64.cpp \
../src/common.cpp \
../src/config.cpp \
../src/handler_proc.cpp \
../src/log.cpp \
../src/loggermutex.cpp \
../src/main_dispatcher.cpp \
../src/mpx.cpp \
../src/msg_error.cpp \
../src/parameters.cpp \
../src/playlist.cpp \
../src/tclientsocket.cpp \
../src/url.cpp 

C_SRCS += \
../src/ini.c 

OBJS += \
./src/INIReader.o \
./src/aac.o \
./src/accordion_V9.x.o \
./src/base64.o \
./src/common.o \
./src/config.o \
./src/handler_proc.o \
./src/ini.o \
./src/log.o \
./src/loggermutex.o \
./src/main_dispatcher.o \
./src/mpx.o \
./src/msg_error.o \
./src/parameters.o \
./src/playlist.o \
./src/tclientsocket.o \
./src/url.o 

C_DEPS += \
./src/ini.d 

CPP_DEPS += \
./src/INIReader.d \
./src/aac.d \
./src/accordion_V9.x.d \
./src/base64.d \
./src/common.d \
./src/config.d \
./src/handler_proc.d \
./src/log.d \
./src/loggermutex.d \
./src/main_dispatcher.d \
./src/mpx.d \
./src/msg_error.d \
./src/parameters.d \
./src/playlist.d \
./src/tclientsocket.d \
./src/url.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


