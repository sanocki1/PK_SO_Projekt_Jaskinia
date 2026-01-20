#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "logger.h"
#include "shared.h"

int main(int argc, char* argv[]) {

    PRINT("I'm the main process!");

    if (argc != 3) {
        PRINT("Wrong number of arguments in the main process");
        return 1;
    }

    int shmid = shmget(IPC_PRIVATE,
                        sizeof(sharedState),
                        IPC_CREAT | 0600);
    char shmidStr[32];
    snprintf(shmidStr, sizeof(shmidStr), "%d", shmid);
    sharedState* state = shmat(shmid, nullptr, 0);
    if (state == (sharedState*)-1) {
        perror("shmat");
    }

    state->Tp = atoi(argv[1]);
    state->Tk = atoi(argv[2]);

    int guardPid = fork();
    if (guardPid == -1) {
        perror("fork");
    }
    if (guardPid == 0) {
        execl("./guard", "guard", shmidStr, NULL);
        perror("execl failed");
    }

    wait(nullptr);

    shmdt(state);
    shmctl(shmid, IPC_RMID, nullptr);

    return 0;
}