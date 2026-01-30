/**
* @file utils.c
* @brief Helper functions for IPC.
*
* This file consists of functions for handling:
* - shared memory
* - message queues
* - semaphores
*/
#include "utils.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>


/** @brief Generates a unique key used for IPC. */
key_t generateKey(int id) {
    key_t key = ftok(".", id);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    return key;
}

/** @brief Creates or opens a shared memory segment. */
int getShmid(key_t shmKey, int flag) {
    int shmid = shmget(shmKey, sizeof(sharedState), flag);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

/** @brief Creates a shared memory segment. */
int createSharedMemory(int keyId) {
    key_t key = generateKey(keyId);
    return getShmid(key, IPC_CREAT | 0600);
}

/** @brief Opens an existing shared memory segment. */
int openSharedMemory(int keyId) {
    key_t key = generateKey(keyId);
    return getShmid(key, 0);
}

/** @brief Attaches to a shared memory segment. */
sharedState* attachSharedMemory(int shmid) {
    sharedState* state = shmat(shmid, NULL, 0);
    if (state == (sharedState*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return state;
}

/** @brief Deattaches from a shared memory segment. */
void deattachSharedMemory(sharedState* state) {
    if (shmdt(state) == -1) {
        perror("shmdt");
    }
}

/** @brief Destroys a shared memory segment. */
void destroySharedMemory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}

/** @brief Creates or opens a message queue. */
int getMsgQueueId(key_t msgKey, int flag) {
    int msgQueueId = msgget(msgKey, flag);
    if (msgQueueId == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    return msgQueueId;
}

/** @brief Creates a message queue. */
int createMsgQueue(int keyId) {
    key_t key = generateKey(keyId);
    return getMsgQueueId(key, IPC_CREAT | 0600);
}

/** @brief Opens an existing message queue. */
int openMsgQueue(int keyId) {
    key_t key = generateKey(keyId);
    return getMsgQueueId(key, 0);
}

/** @brief Removes a message queue. */
void destroyMsgQueue(int msgQueueId) {
    if (msgctl(msgQueueId, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
    }
}

/** @brief Creates or opens a semaphore set. */
int getSemaphoreId(key_t semKey, int count, int flag) {
    int semId = semget(semKey, count, flag);
    if (semId == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    return semId;
}

/** @brief Creates a semaphore set. */
int createSemaphore(int keyId, int count) {
    key_t key = generateKey(keyId);
    return getSemaphoreId(key, count, IPC_CREAT | 0600);
}

/** @brief Opens an existing semaphore set. */
int openSemaphore(int keyId, int count) {
    key_t key = generateKey(keyId);
    return getSemaphoreId(key, count, 0);
}

/** @brief Initializes semaphores value. */
void initializeSemaphore(int semId, int semNum ,int initialValue) {
    if (semctl(semId, semNum, SETVAL, initialValue) == -1) {
        perror("semctl SETVAL");
        exit(EXIT_FAILURE);
    }
}

/** @brief Destroys a semaphore set. */
void destroySemaphore(int semId) {
    if (semctl(semId, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
    }
}

/** @brief Wait operation on semaphore. */
void P(int semId, int semnum) {
    struct sembuf op = {semnum, -1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop P");
            exit(EXIT_FAILURE);
        }
    }
}

/** @brief Signal operation on semaphore. */
void V(int semId, int semnum) {
    struct sembuf op = {semnum, +1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop V");
            exit(EXIT_FAILURE);
        }
    }
}