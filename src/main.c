#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
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

    srand(time(nullptr) + getpid());

    if (argc != 3) {
        PRINT_ERR("Wrong number of arguments in the main process");
        return 1;
    }

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, IPC_CREAT | 0600);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(VISITOR_CASHIER_QUEUE_KEY_ID);
    int visitorCashierMsgQueueId = getMsgQueueId(key, IPC_CREAT | 0600);

    key = generateKey(VISITOR_GUIDE_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId = getMsgQueueId(key, IPC_CREAT | 0600);

    key = generateKey(SEMAPHORE_KEY_ID);
    int semId = getSemaphoreId(key, SEM_COUNT, IPC_CREAT | 0600);
    initializeSemaphore(semId,BRIDGE_SEM, BRIDGE_CAPACITY);
    initializeSemaphore(semId,VISITOR_COUNT_SEM, 1);
    initializeSemaphore(semId,GUIDE_BRIDGE_WAIT_SEM, 0);

    state->Tp = atoi(argv[1]);
    state->Tk = atoi(argv[2]);
    state->closing = 0;
    state->moneyEarned = 0;
    state->ticketsSold = 0;

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
    char guidePidStr[16];
    snprintf(guidePidStr, sizeof(guidePidStr), "%d", guidePid);

    pid_t guardPid = fork();
    if (guardPid == -1) {
        PRINT_ERR("guard fork");
    }
    if (guardPid == 0) {
        execl("./guard", "guard", cashierPidStr, guidePidStr, NULL);
        PRINT_ERR("guard execl failed");
    }

    while (!state->closing) {
        int groupCount = rand() % MAX_VISITOR_GROUP_SIZE + 1;
        for (int i = 0; i < groupCount; i++) {
            pid_t visitorPid = fork();
            if (visitorPid == -1) {
                PRINT_ERR("visitor fork");
            }
            if (visitorPid == 0) {
                execl("./visitor", "visitor", argv[1],   NULL);
                PRINT_ERR("visitor execl failed");
            }
        }
        sleep(VISITOR_FREQUENCY);
    }

    while (wait(nullptr) > 0) {}

    PRINT("Tickets sold for the day: %d", state->ticketsSold);
    PRINT("Total money earned: %.2f", state->moneyEarned);

    destroyMsgQueue(visitorCashierMsgQueueId);
    destroyMsgQueue(visitorGuideMsgQueueId);
    deattachSharedMemory(state);
    destroySharedMemory(shmid);
    destroySemaphore(semId);

    PRINT("Finishing...");
    return 0;
}
