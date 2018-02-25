#ifndef MISC_H_
#define MISC_H_
// #include "pins.h"
// #include <avr/wdt.h>

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

#define DIGIT2ASCII(x) (x+48)

#define SEND_STRING(s) CDC_Device_SendString(&VirtualSerial_CDC_Interface, s)
#define SEND_CHAR(c)   CDC_Device_SendByte(&VirtualSerial_CDC_Interface, c)

#define REPLY(M, ...)  SEND_STRING(M "\r" ##__VA_ARGS__)
#define REPLYs(s)  SEND_STRING(s)
#define REPLYn(...) SEND_STRING(##__VA_ARGS__)
#define REPLYF(M, ...) printf(M "\r", ##__VA_ARGS__)
#define REPLYF_raw(M, ...) printf(M, ##__VA_ARGS__)

#ifdef __AVR_ARCH__
//declared in main.c
#include <LUFA/Drivers/USB/USB.h>
extern USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface;
#endif

#define static_assert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#endif
