SOURCES := \
    delay.c \
    log.c \
    main.c \
    systick.c \
    ../Project_Settings/Startup_Code/startup_MKE06Z4.S \
    ../Project_Settings/Startup_Code/system_MKE06Z4.c \
    ../../common/FatFS/diskio.c \
    ../../common/FatFS/ff.c \
    ../../common/FatFS/mmc_spi.c \
    ../../common/FatFS/clock.c \
    ../../common/SEGGER/SEGGER_RTT.c \
    ../../common/SEGGER/SEGGER_RTT_printf.c \
    ../../common/adc.c \
    ../../common/bitbang_uart.c \
    ../../common/cifra/aes.c \
    ../../common/cifra/blockwise.c \
    ../../common/cifra/cmac.c \
    ../../common/cifra/gf128.c \
    ../../common/cifra/modes.c \
    ../../common/crc.c \
    ../../common/flash.c \
    ../../common/keys.c \
    ../../common/proginfo.c \
    ../../common/spi0.c \
    ../../common/timer.c \
    ../../common/crash_handler.c \
    ../../common/power.c

CFLAGS := $(COMMON_FLAGS) $(INCLUDE) $(DEFINES) $(OPT) $(CFLAGS_EXTRA)
