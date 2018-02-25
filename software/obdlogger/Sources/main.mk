SOURCES := \
    FreeRTOS/croutine.c \
    FreeRTOS/event_groups.c \
    FreeRTOS/list.c \
    FreeRTOS/port/port.c \
    FreeRTOS/queue.c \
    FreeRTOS/tasks.c \
    FreeRTOS/timers.c \
    acquisition_task.c \
    application_tasks.c \
    debug.c \
    diagnostics.c \
    gps_uart.c \
    led.c \
    logger_core.c \
    main.c \
    minmea.c \
    obd/obd.c \
    obd/obd_can.c \
    obd/obd_k_line.c \
    obd/obd_pids.c \
    obd/obd_uart.c \
    watchdog.c \
    rtt_console.c \
    gps_core.c \
    storage_task.c \
    ../Project_Settings/Startup_Code/startup_MKE06Z4.S \
    ../Project_Settings/Startup_Code/system_MKE06Z4.c \
    ../../common/FatFS/diskio.c \
    ../../common/FatFS/ff.c \
    ../../common/FatFS/mmc_spi.c \
    ../../common/FatFS/clock.c \
    ../../common/SEGGER/SEGGER_RTT.c \
    ../../common/SEGGER/SEGGER_RTT_printf.c \
    ../../common/SEGGER/SEGGER_SYSVIEW.c \
    ../../common/SEGGER/SEGGER_SYSVIEW_Config_FreeRTOS.c \
    ../../common/SEGGER/SEGGER_SYSVIEW_FreeRTOS.c \
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

