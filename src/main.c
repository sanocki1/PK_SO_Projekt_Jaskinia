#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/shm.h>
#include "config.h"
#include "logger.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


int main(int argc, char* argv[]) {
    PRINT("I'm the main process!");

    if (argc != 3) {
        PRINT_ERR("Wrong number of arguments in the main process");
        return 1;
    }

    // key_t semKey = generateKey(TEST_SEMAPHORE);
    // int sem = semget(semKey, 1, IPC_CREAT | 0600);

    key_t shmKey = generateKey(SHM_KEY);
    int shmid = getShmid(shmKey, IPC_CREAT | 0600);
    sharedState* state = getSharedMemory(shmid);

    state->Tp = atoi(argv[1]);
    state->Tk = atoi(argv[2]);
    state->closing = 0;

    key_t visitorCashierMsgKey = generateKey(VISITOR_CASHIER_QUEUE);
    int visitorCashierMsgQueueId = getMsgQueueId(visitorCashierMsgKey, IPC_CREAT | 0600);

    key_t visitorGuideMsgKey = generateKey(VISITOR_GUIDE_QUEUE);
    int visitorGuideMsgQueueId = getMsgQueueId(visitorGuideMsgKey, IPC_CREAT | 0600);

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

    pid_t guidePid = fork();
    if (guidePid == -1) {
        PRINT_ERR("guide fork");
    }
    if (guidePid == 0) {
        execl("./guide", "guide", NULL);
        PRINT_ERR("guide execl failed");
    }

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

    destroyMsgQueue(visitorCashierMsgQueueId);
    destroyMsgQueue(visitorGuideMsgQueueId);
    deattachSharedMemory(state);
    destroySharedMemory(shmid);

    PRINT("Finishing...");
    return 0;
}
