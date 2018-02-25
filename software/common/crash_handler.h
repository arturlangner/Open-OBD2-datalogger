#pragma once
#include <string.h> //for strcpy

char GLOBAL_news_from_the_grave[64];

void crash_handler_init(void);
char* crash_handler_get_info(void);
