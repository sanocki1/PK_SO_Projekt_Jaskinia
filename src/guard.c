#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "logger.h"
#include "config.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    PRINT("I'm the guard!");

    key_t shmKey = generateKey(SHM_KEY_ID);
    int shmid = getShmid(shmKey, 0);
    sharedState* state = attachSharedMemory(shmid);

    int Tp = state->Tp;
    int Tk = state->Tk;
    int secondsOpen = (Tk - Tp) * SECONDS_PER_HOUR;

    pid_t cashierPid = atoi(argv[1]);
    pid_t guide1Pid = atoi(argv[2]);
    pid_t guide2Pid = atoi(argv[3]);

    PRINT("Tour is open!");
    PRINT("Tour will close in %d seconds", secondsOpen);
    sleep(secondsOpen);
    state->closing = 1;
    PRINT("Tour is closed!");

    kill(cashierPid, SIGTERM);
    kill(guide1Pid, SIGTERM);
    kill(guide2Pid, SIGTERM);
    deattachSharedMemory(state);

    PRINT("Finishing...");
    return 0;
}
