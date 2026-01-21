#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>

#include "logger.h"
#include "shared.h"

int main(int argc, char* argv[]) {
    PRINT("I'm the cashier!");

    int shmid = atoi(argv[1]);
    sharedState* state = shmat(shmid, nullptr, 0);
    if (state == (sharedState*)-1) {
        perror("shmat");
    }

    while (!state->closing) {
        pid_t visitorPid = fork();
        if (visitorPid == -1) {
            perror("visitor fork");
        }
        if (visitorPid == 0) {
            execl("./visitor", "visitor", argv[1],   NULL);
            perror("visitor execl failed");
        }
        sleep(1);
    }

    PRINT("Finishing...");
}
