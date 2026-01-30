/**
* @file logger.c
* @brief Simulation logger.
*
* Provides safe and synchronized access to a file and the standard output stream.
*/
#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "../src/utils.h"

#define LOG_FILE "../logs/simulation.txt"

static int logSemId = -1;

/** @brief Initializes the logger, sets semaphore from the user process. */
void initLogger(int semId) {
    logSemId = semId;
}

/** @brief Cleans the log file. */
void clearLog() {
    FILE* logFile = fopen(LOG_FILE, "w");
    if (logFile != NULL) {
        fclose(logFile);
    }
}

/** @brief Writes to a log file and the standard output stream. */
void writeLog(const char* color, const char* processName, pid_t pid,
                     const char* fmt, va_list args) {
    if (logSemId == -1) {
        fprintf(stderr, "Logger not initialized\n");
        return;
    }

    P(logSemId, LOG_SEM);

    FILE* logFile = fopen(LOG_FILE, "a");
    if (logFile == NULL) {
        perror("fopen");
        V(logSemId, LOG_SEM);
        return;
    }

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    fprintf(logFile, "%s (PID: %d) %s\n", processName, pid, buffer);
    fclose(logFile);

    printf("%s%s (PID: %d) %s\n", color, processName, pid, buffer);

    V(logSemId, LOG_SEM);
}

/** @brief Logs a normal communication. */
void logMessage(const char* color, const char* processName,
                const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeLog(color, processName, getpid(), fmt, args);
    va_end(args);
}

/** @brief Logs an error. */
void logError(const char* processName, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeLog(COLOR_RED, processName, getpid(), fmt, args);
    va_end(args);
}