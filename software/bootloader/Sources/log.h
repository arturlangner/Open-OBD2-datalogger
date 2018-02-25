#ifndef LOG_H_
#define LOG_H_
#include <stdint.h>
#include <stdio.h>

void log_start(void);

#define log_entry(s) log_entry_(__LINE__, s)
void log_entry_(uint32_t line, char *s);

void log_close(void);

#define log_format(M, ...) do { \
							char scratchpad[128]; \
							sprintf(scratchpad, M, ##__VA_ARGS__); \
							log_entry(scratchpad); \
							} while (0)

#endif /* LOG_H_ */
