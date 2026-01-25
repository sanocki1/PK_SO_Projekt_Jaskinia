#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
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
    if (Tp < 0 || Tk > 24 || Tp >= Tk || SECONDS_PER_HOUR < 1) {
        printf("Invalid time parameters\n");
        return 1;
    }
    int secondsOpen = (Tk - Tp) * SECONDS_PER_HOUR;

    pid_t cashierPid = atoi(argv[1]);

    PRINT("Tour is open!");
    PRINT("Tour will close in %d seconds", secondsOpen);
    sleep(secondsOpen);
    state->closing = 1;
    PRINT("Tour is closed!");

    kill(cashierPid, SIGINT);
    deattachSharedMemory(state);

    PRINT("Finishing...");
    return 0;
}
