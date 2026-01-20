#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include "logger.h"
#include "config.h"
#include "shared.h"


int main(int argc, char* argv[]) {
    PRINT("I'm the guard!");

    if (argc != 2) {
        PRINT("Wrong number of arguments in the guard process");
        return 1;
    }

    int shmid = atoi(argv[1]);
    sharedState* state = shmat(shmid, nullptr, 0);
    if (state == (sharedState*)-1) {
        perror("shmat");
    }

    int Tp = state->Tp;
    int Tk = state->Tk;
    if (Tp < 0 || Tk > 24 || Tp >= Tk || SECONDS_PER_HOUR < 1) {
        printf("Invalid time parameters\n");
        return 1;
    }
    int secondsOpen = (Tk - Tp) * SECONDS_PER_HOUR;

    PRINT("Tour is open!");
    PRINT("Tour will close in %d seconds", secondsOpen);
    sleep(secondsOpen);
    PRINT("Tour is closed!");

    shmdt(state);

    return 0;
}
