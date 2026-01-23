#include "utils.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/shm.h>

key_t generateKey(int id) {
    key_t key = ftok(".", id);
    if (key == -1) {
        PRINT_ERR("ftok");
        exit(1);
    }
    return key;
}

int getShmid(key_t shmKey, int flag) {
    int shmid = shmget(shmKey, sizeof(sharedState), flag);
    if (shmid == -1) {
        PRINT_ERR("shmget");
        exit(1);
    }
    return shmid;
}

sharedState* getSharedMemory(int shmid) {
    sharedState* state = shmat(shmid, nullptr, 0);
    if (state == (sharedState*)-1) {
        PRINT_ERR("shmat");
        exit(1);
    }
    return state;
}

int getMsgQueueId(key_t msgKey, int flag) {
    int msgQueueId = msgget(msgKey, flag);
    if (msgQueueId == -1) {
        PRINT_ERR("msgget");
        exit(1);
    }
    return msgQueueId;
}

void destroyMsgQueue(int msgQueueId) {
    if (msgctl(msgQueueId, IPC_RMID, nullptr) == -1) {
        PRINT_ERR("msgctl");
        exit(1);
    }
}

void deattachSharedMemory(sharedState* state) {
    if (shmdt(state) == -1) {
        PRINT_ERR("shmdt");
        exit(1);
    }
}

void destroySharedMemory(int shmid) {
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        PRINT_ERR("shmctl");
        exit(1);
    }
}