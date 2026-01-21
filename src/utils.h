#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SHM_KEY 1
#define VISITOR_CASHIER_MSG 2

typedef struct {
    int Tp; // opening time
    int Tk; // closing time
    int closing; // 0 - simulation running, 1 - simulation closing
} sharedState;

typedef struct {
    long mtype;
    int age;
    int route;
    int isRepeat;
} TicketMessage;

key_t generateKey(int id);

int getShmid(key_t shmKey, int flag);

sharedState* getSharedMemory(int shmid);

int getMsgQueueId(key_t msgKey, int flag);

#endif