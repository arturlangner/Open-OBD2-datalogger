BUILD_DIR  := build/targets

TARGET_DIR := build

SOURCES := \
    main.c \
    ../common/cifra/gf128.c \
    ../common/cifra/blockwise.c \
    ../common/cifra/cmac.c \
    ../common/cifra/modes.c \
    ../common/cifra/aes.c \
    ../common/keys.c

TARGET :=sign_firmware
