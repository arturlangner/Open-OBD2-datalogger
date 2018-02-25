CC = arm-none-eabi-gcc
SIZE = arm-none-eabi-size
OBJCOPY = arm-none-eabi-objcopy

COMMON_FLAGS = -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -g

COMMON_LDFLAGS = -Wl,--gc-sections

CFLAGS_EXTRA = -std=gnu11 -Wall -pipe -ffunction-sections -fdata-sections

OPT = -O3

DEFINES = -DBOOTLOADER_BUILD=0 -DDEBUG_RTT_ENABLE=1

BUILD_DIR  := build/targets

TARGET_DIR := build

LIBC_SPECS = --specs=nano.specs -specs=nosys.specs

INCLUDE += -I Includes -I Sources -I Sources/FreeRTOS/include -I Sources/FreeRTOS/port -I ../common

CFLAGS := $(COMMON_FLAGS) $(INCLUDE) $(DEFINES) $(OPT) $(CFLAGS_EXTRA)

SUBMAKEFILES := application-bootloadable.mk application-standalone.mk

define postmake
	$(OBJCOPY) $(TARGET_DIR)/$(TARGET) -O binary $(TARGET_DIR)/$(basename $(TARGET)).bin
	$(OBJCOPY) $(TARGET_DIR)/$(TARGET) -O ihex $(TARGET_DIR)/$(basename $(TARGET)).hex
	$(SIZE) $(TARGET_DIR)/$(TARGET)
endef

define postclean
	rm -f $(TARGET_DIR)/$(basename $(TARGET)).bin
	rm -f $(TARGET_DIR)/$(basename $(TARGET)).hex
	rm -Rf $(BUILD_DIR)/$(TARGET)
endef
