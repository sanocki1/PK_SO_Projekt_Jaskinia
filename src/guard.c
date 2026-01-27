#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "logger.h"
#include "config.h"
#include "utils.h"

void terminateProcesses(pid_t cashierPid, pid_t guide1Pid, pid_t guide2Pid);

int main(int argc, char* argv[]) {
    PRINT("I'm the guard!");

    int shmid = openSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int secondsOpen = (state->Tk - state->Tp) * SECONDS_PER_HOUR;

    pid_t cashierPid = atoi(argv[1]);
    pid_t guide1Pid = atoi(argv[2]);
    pid_t guide2Pid = atoi(argv[3]);

    PRINT("Tour is open!");
    PRINT("Tour will close in %d seconds", secondsOpen);
    sleep(secondsOpen);
    state->closing = 1;
    PRINT("Tour is closed!");

    terminateProcesses(cashierPid, guide1Pid, guide2Pid);;
    deattachSharedMemory(state);

    PRINT("Finishing...");
    return 0;
}

void terminateProcesses(pid_t cashierPid, pid_t guide1Pid, pid_t guide2Pid) {
    kill(cashierPid, SIGTERM);
    kill(guide1Pid, SIGTERM);
    kill(guide2Pid, SIGTERM);
}