#include "utils.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
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

int createSharedMemory(int keyId) {
    key_t key = generateKey(keyId);
    return getShmid(key, IPC_CREAT | 0600);
}

int openSharedMemory(int keyId) {
    key_t key = generateKey(keyId);
    return getShmid(key, 0);
}

sharedState* attachSharedMemory(int shmid) {
    sharedState* state = shmat(shmid, NULL, 0);
    if (state == (sharedState*)-1) {
        PRINT_ERR("shmat");
        exit(1);
    }
    return state;
}

void deattachSharedMemory(sharedState* state) {
    if (shmdt(state) == -1) {
        PRINT_ERR("shmdt");
        exit(1);
    }
}

void destroySharedMemory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        PRINT_ERR("shmctl");
        exit(1);
    }
}

int getMsgQueueId(key_t msgKey, int flag) {
    int msgQueueId = msgget(msgKey, flag);
    if (msgQueueId == -1) {
        PRINT_ERR("msgget");
        exit(1);
    }
    return msgQueueId;
}

int createMsgQueue(int keyId) {
    key_t key = generateKey(keyId);
    return getMsgQueueId(key, IPC_CREAT | 0600);
}

int openMsgQueue(int keyId) {
    key_t key = generateKey(keyId);
    return getMsgQueueId(key, 0);
}

void destroyMsgQueue(int msgQueueId) {
    if (msgctl(msgQueueId, IPC_RMID, NULL) == -1) {
        PRINT_ERR("msgctl");
        exit(1);
    }
}

int getSemaphoreId(key_t semKey, int count, int flag) {
    int semId = semget(semKey, count, flag);
    if (semId == -1) {
        PRINT_ERR("semget");
        exit(1);
    }
    return semId;
}

int createSemaphore(int keyId, int count) {
    key_t key = generateKey(keyId);
    return getSemaphoreId(key, count, IPC_CREAT | 0600);
}

int openSemaphore(int keyId, int count) {
    key_t key = generateKey(keyId);
    return getSemaphoreId(key, count, 0);
}

void initializeSemaphore(int semId, int semNum ,int initialValue) {
    if (semctl(semId, semNum, SETVAL, initialValue) == -1) {
        PRINT_ERR("semctl");
        exit(1);
    }
}

void destroySemaphore(int semId) {
    if (semctl(semId, 0, IPC_RMID) == -1) {
        PRINT_ERR("semctl");
        exit(1);
    }
}

void P(int semId, int semnum) {
    struct sembuf op = {semnum, -1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            PRINT_ERR("semop P");
            exit(EXIT_FAILURE);
        }
    }
}

void V(int semId, int semnum) {
    struct sembuf op = {semnum, +1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            PRINT_ERR("semop P");
            exit(EXIT_FAILURE);
        }
    }
}