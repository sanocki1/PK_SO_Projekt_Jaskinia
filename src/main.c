#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "config.h"
#include "logger.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    PRINT("I'm the main process!");

    if (argc != 3) {
        PRINT("Wrong number of arguments in the main process");
        return 1;
    }

    key_t shmKey = generateKey(SHM_KEY);
    int shmid = getShmid(shmKey, IPC_CREAT | 0600);
    sharedState* state = getSharedMemory(shmid);

    state->Tp = atoi(argv[1]);
    state->Tk = atoi(argv[2]);
    state->closing = 0;

    key_t msgKey = generateKey(VISITOR_CASHIER_MSG);
    int msgQueueId = getMsgQueueId(msgKey, IPC_CREAT | 0600);

    pid_t cashierPid = fork();
    if (cashierPid == -1) {
        PRINT_ERR("cashier fork");
    }
    if (cashierPid == 0) {
        execl("./cashier", "cashier", NULL);
        PRINT_ERR("cashier execl failed");
    }

    char cashierPidStr[16];
    snprintf(cashierPidStr, sizeof(cashierPidStr), "%d", cashierPid);

    pid_t guardPid = fork();
    if (guardPid == -1) {
        PRINT_ERR("guard fork");
    }
    if (guardPid == 0) {
        execl("./guard", "guard", cashierPidStr, NULL);
        PRINT_ERR("guard execl failed");
    }

    while (!state->closing) {
        pid_t visitorPid = fork();
        if (visitorPid == -1) {
            PRINT_ERR("visitor fork");
        }
        if (visitorPid == 0) {
            execl("./visitor", "visitor", argv[1],   NULL);
            PRINT_ERR("visitor execl failed");
        }
        sleep(VISITOR_FREQUENCY);
    }

    while (wait(nullptr) > 0) {}

    if (msgctl(msgQueueId, IPC_RMID, nullptr) == -1) {
        PRINT_ERR("msgctl");
    }

    if (shmdt(state) == -1) {
        PRINT_ERR("shmdt");
    }

    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        PRINT_ERR("shmctl");
    }

    PRINT("Finishing...");
    return 0;
}
