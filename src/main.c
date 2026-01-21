#include <errno.h>
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
    state->closing = 0;

    pid_t guardPid = fork();
    if (guardPid == -1) {
        perror("guard fork");
    }
    if (guardPid == 0) {
        execl("./guard", "guard", shmidStr, NULL);
        perror("guard execl failed");
    }

    pid_t cashierPid = fork();
    if (cashierPid == -1) {
        perror("cashier fork");
    }
    if (cashierPid == 0) {
        execl("./cashier", "cashier", shmidStr, NULL);
        perror("cashier execl failed");
    }

    while (wait(nullptr) > 0) {}
    // if (errno != ECHILD) {
    //     perror("wait");
    // }


    shmdt(state);
    shmctl(shmid, IPC_RMID, nullptr);

    PRINT("Finishing...");
    return 0;
}