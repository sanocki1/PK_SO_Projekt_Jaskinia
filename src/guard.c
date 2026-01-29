/**
* @file guard.c
* @brief Proces strażnika.
*
* Proces:
* - kontroluje czasu otwarcia jaskini
* - ustawia flagę zamknięcia symulacji
* - kończy procesy kasjera i przewodników
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "logger.h"
#include "config.h"
#include "utils.h"

/** @brief Wysyła sygnały zakończenia do procesów symulacji. */
void terminateProcesses(pid_t cashierPid, pid_t guide1Pid, pid_t guide2Pid);

int main(int argc, char* argv[]) {
    if (argc < 4) {
        perror("Not enough arguments");
        exit(EXIT_FAILURE);
    }
    int shmid = openSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int semId = openSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initLogger(semId);
    LOG("I'm the guard!");

    int secondsOpen = (state->Tk - state->Tp) * SECONDS_PER_HOUR;

    pid_t cashierPid = atoi(argv[1]);
    pid_t guide1Pid = atoi(argv[2]);
    pid_t guide2Pid = atoi(argv[3]);

    LOG("Touring is open!");
    LOG("Touring will close in %d seconds", secondsOpen);

    // tracking elapsed time in short intervals so the guard does not "keep counting time" when simulation is paused
    for (int elapsed = 0; elapsed < secondsOpen; elapsed++) {
        sleep(1);
    };

    state->closing = 1;
    LOG("Touring is closing!");

    terminateProcesses(cashierPid, guide1Pid, guide2Pid);;
    deattachSharedMemory(state);

    LOG("Finishing...");
    return 0;
}

void terminateProcesses(pid_t cashierPid, pid_t guide1Pid, pid_t guide2Pid) {
    if (kill(cashierPid, SIGTERM) == -1) { perror("kill cashier"); }
    if (kill(guide1Pid, SIGTERM) == -1) { perror("kill guide1"); }
    if (kill(guide2Pid, SIGTERM) == -1) { perror("kill guide2"); }
}