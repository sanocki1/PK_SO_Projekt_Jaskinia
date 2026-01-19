#ifndef LOGGER_H
#define LOGGER_H

#ifndef PROCESS_NAME
#define PROCESS_NAME "[Unknown]"
#endif

#define PRINT(fmt, ...) \
printf(PROCESS_NAME " " fmt "\n", ##__VA_ARGS__)

#endif