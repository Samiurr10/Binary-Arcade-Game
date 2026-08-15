######################################
# Binary Arcade — mbed-style STM32F401 firmware
# Target: STM32F401CC (Cortex-M4) — closest Simantic MCU to classic mbed M3
######################################

TARGET = binary-arcade-firmware
BUILD_DIR = build

# Prefer portable Arm GNU toolchain if present
ARM_GNU ?= $(HOME)/toolchains/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin
ifneq ($(wildcard $(ARM_GNU)/arm-none-eabi-g++),)
  PREFIX := $(ARM_GNU)/arm-none-eabi-
else
  PREFIX := arm-none-eabi-
endif

CXX = $(PREFIX)g++
CC  = $(PREFIX)gcc
AS  = $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY = $(PREFIX)objcopy
SIZE = $(PREFIX)size

ROOT := ..
LDSCRIPT = STM32F401XX_FLASH.ld

CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
DEFS = -DSTM32F401xC -DBINARY_ARCADE_EMBEDDED
INCLUDES = -Iplatform -I$(ROOT)/include

COMMON = $(CPU) $(DEFS) $(INCLUDES) -Os -ffunction-sections -fdata-sections \
	-fno-exceptions -fno-rtti -Wall -Wextra

CFLAGS = $(COMMON) -std=c11
CXXFLAGS = $(COMMON) -std=c++20
LDFLAGS = $(CPU) -T$(LDSCRIPT) -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref

LIB_SRCS = \
	$(ROOT)/src/framebuffer.cpp \
	$(ROOT)/src/game.cpp \
	$(ROOT)/src/bus.cpp \
	$(ROOT)/src/flash_iap.cpp \
	$(ROOT)/src/system.cpp

FW_CXX_SRCS = \
	platform/system_init.cpp \
	platform/mbed_serial.cpp \
	src/main.cpp

FW_C_SRCS = \
	platform/syscalls.c \
	platform/sysmem.c

ASM_SRCS = startup_stm32f401xc.s

OBJS = \
	$(patsubst $(ROOT)/src/%.cpp,$(BUILD_DIR)/lib_%.o,$(LIB_SRCS)) \
	$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(FW_CXX_SRCS)) \
	$(patsubst %.c,$(BUILD_DIR)/%.o,$(FW_C_SRCS)) \
	$(BUILD_DIR)/startup.o

.PHONY: all clean size

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin size

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/platform $(BUILD_DIR)/src

$(BUILD_DIR)/lib_%.o: $(ROOT)/src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/platform/%.o: platform/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/src/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/platform/%.o: platform/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.o: $(ASM_SRCS) | $(BUILD_DIR)
	$(AS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -lc -lm -lstdc++ -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD_DIR)
