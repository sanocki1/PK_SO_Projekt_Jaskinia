/**
* @file utils.c
* @brief Funkcje pomocnicze IPC.
*
* Plik zawiera funkcje do obsługi:
* - pamięci współdzielonej
* - kolejek komunikatów
* - semaforów
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

/** @brief Tworzy unikalny klucz wykorzystywany w IPC. */
key_t generateKey(int id) {
    key_t key = ftok(".", id);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    return key;
}

/** @brief Tworzy lub otwiera segment pamięci współdzielonej. */
int getShmid(key_t shmKey, int flag) {
    int shmid = shmget(shmKey, sizeof(sharedState), flag);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

/** @brief Tworzy segment pamięci współdzielonej. */
int createSharedMemory(int keyId) {
    key_t key = generateKey(keyId);
    return getShmid(key, IPC_CREAT | 0600);
}

/** @brief Otwiera istniejący segment pamięci współdzielonej. */
int openSharedMemory(int keyId) {
    key_t key = generateKey(keyId);
    return getShmid(key, 0);
}

/** @brief Dołącza pamięć współdzieloną do procesu. */
sharedState* attachSharedMemory(int shmid) {
    sharedState* state = shmat(shmid, NULL, 0);
    if (state == (sharedState*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return state;
}

/** @brief Odłącza pamięć współdzieloną od procesu. */
void deattachSharedMemory(sharedState* state) {
    if (shmdt(state) == -1) {
        perror("shmdt");
    }
}

/** @brief Usuwa segment pamięci współdzielonej. */
void destroySharedMemory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}

/** @brief Tworzy lub otwiera kolejke komunikatów */
int getMsgQueueId(key_t msgKey, int flag) {
    int msgQueueId = msgget(msgKey, flag);
    if (msgQueueId == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    return msgQueueId;
}

/** @brief Tworzy kolejkę komunikatów. */
int createMsgQueue(int keyId) {
    key_t key = generateKey(keyId);
    return getMsgQueueId(key, IPC_CREAT | 0600);
}

/** @brief Otwiera istniejącą kolejkę komunikatów. */
int openMsgQueue(int keyId) {
    key_t key = generateKey(keyId);
    return getMsgQueueId(key, 0);
}

/** @brief Usuwa kolejkę komunikatów. */
void destroyMsgQueue(int msgQueueId) {
    if (msgctl(msgQueueId, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
    }
}

/** @brief Tworzy lub otwiera zestaw semaforów. */
int getSemaphoreId(key_t semKey, int count, int flag) {
    int semId = semget(semKey, count, flag);
    if (semId == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    return semId;
}

/** @brief Tworzy zestaw semaforów. */
int createSemaphore(int keyId, int count) {
    key_t key = generateKey(keyId);
    return getSemaphoreId(key, count, IPC_CREAT | 0600);
}

/** @brief Otwiera istniejący zestaw semaforów. */
int openSemaphore(int keyId, int count) {
    key_t key = generateKey(keyId);
    return getSemaphoreId(key, count, 0);
}

/** @brief Inicjalizuje wartość semafora. */
void initializeSemaphore(int semId, int semNum ,int initialValue) {
    if (semctl(semId, semNum, SETVAL, initialValue) == -1) {
        perror("semctl SETVAL");
        exit(EXIT_FAILURE);
    }
}

/** @brief Usuwa zestaw semaforów. */
void destroySemaphore(int semId) {
    if (semctl(semId, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
    }
}

/** @brief Operacja wait na semaforze */
void P(int semId, int semnum) {
    struct sembuf op = {semnum, -1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop P");
            exit(EXIT_FAILURE);
        }
    }
}

/** @brief Operacja signal na semaforze */
void V(int semId, int semnum) {
    struct sembuf op = {semnum, +1, 0};
    if (semop(semId, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop V");
            exit(EXIT_FAILURE);
        }
    }
}