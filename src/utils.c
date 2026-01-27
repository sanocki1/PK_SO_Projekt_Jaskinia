#include "utils.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

key_t generateKey(int id) {
    key_t key = ftok(".", id);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    return key;
}

int getShmid(key_t shmKey, int flag) {
    int shmid = shmget(shmKey, sizeof(sharedState), flag);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
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
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return state;
}

void deattachSharedMemory(sharedState* state) {
    if (shmdt(state) == -1) {
        perror("shmdt");
    }
}

void destroySharedMemory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}

int getMsgQueueId(key_t msgKey, int flag) {
    int msgQueueId = msgget(msgKey, flag);
    if (msgQueueId == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
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
        perror("msgctl IPC_RMID");
    }
}

int getSemaphoreId(key_t semKey, int count, int flag) {
    int semId = semget(semKey, count, flag);
    if (semId == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
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
        perror("semctl SETVAL");
        exit(EXIT_FAILURE);
    }
}

void destroySemaphore(int semId) {
    if (semctl(semId, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
    }
}

void P(int semId, int semnum) {
    struct sembuf op = {semnum, -1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop P");
            exit(EXIT_FAILURE);
        }
    }
}

void V(int semId, int semnum) {
    struct sembuf op = {semnum, +1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop V");
            exit(EXIT_FAILURE);
        }
    }
}