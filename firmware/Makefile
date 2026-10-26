##########################################################################################################################
# Based on the CubeMX project generator Makefile; extended for the layered structure
# (Platform / Devices / App) and stricter warnings for own code.
##########################################################################################################################

######################################
# target
######################################
TARGET = firmware

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization: -Os, because soft-float math and printf need flash space (32 KB)
OPT = -Os

#######################################
# paths
#######################################
BUILD_DIR = build

######################################
# source
######################################
# Vendor / generated code
C_SOURCES =  \
Core/Src/main.c \
Core/Src/stm32c0xx_it.c \
Core/Src/stm32c0xx_hal_msp.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_cortex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_flash.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_flash_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_gpio.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_dma.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_dma_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_pwr.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_pwr_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_exti.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_i2c.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_i2c_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_adc.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_adc_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_tim.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_tim_ex.c \
Core/Src/system_stm32c0xx.c

# Own code: layers Platform -> Devices / App
OWN_PLATFORM_SOURCES = Platform/platform.c
OWN_PURE_SOURCES = \
Devices/mpu6050.c \
App/motion.c \
App/debounce.c \
App/app.c

C_SOURCES += $(OWN_PLATFORM_SOURCES) $(OWN_PURE_SOURCES)

# ASM sources
ASM_SOURCES =  \
startup_stm32c031xx.s

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
CPU=-mcpu=cortex-m0plus
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

AS_DEFS =

C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32C031xx

AS_INCLUDES =

# Vendor headers via -isystem: their warnings are suppressed on purpose (vendor code).
C_INCLUDES =  \
-ICore/Inc \
-isystem Drivers/STM32C0xx_HAL_Driver/Inc \
-isystem Drivers/STM32C0xx_HAL_Driver/Inc/Legacy \
-isystem Drivers/CMSIS/Device/ST/STM32C0xx/Include \
-isystem Drivers/CMSIS/Include \
-IPlatform \
-IDevices \
-IApp

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# Own code is built with -Wextra; Devices/ and App/ also with -Werror (no warnings allowed).
OWN_PURE_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(OWN_PURE_SOURCES:.c=.o)))
OWN_PLATFORM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(OWN_PLATFORM_SOURCES:.c=.o)))
$(OWN_PURE_OBJECTS): CFLAGS += -Wextra -Werror
$(OWN_PLATFORM_OBJECTS): CFLAGS += -Wextra

#######################################
# LDFLAGS
#######################################
LDSCRIPT = STM32C031C6Tx_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir -p $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
