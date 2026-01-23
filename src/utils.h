#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SHM_KEY 1
#define VISITOR_CASHIER_QUEUE 2
#define TEST_SEMAPHORE 3
#define VISITOR_GUIDE_QUEUE 4

#define PRIORITY_REPEAT 1
#define PRIORITY_NORMAL 2

typedef struct {
    int Tp; // opening time
    int Tk; // closing time
    int closing; // 0 - simulation running, 1 - simulation closing
    int adultsOnRouteOneAmount;
    int adultsOnRouteTwoAmount;
} sharedState;

typedef struct {
    long mtype;
    int age;
    int route;
    int isRepeat;
} TicketMessage;

typedef struct {
    long mtype;
    int ppid;
} QueueMessage;

key_t generateKey(int id);

int getShmid(key_t shmKey, int flag);

sharedState* getSharedMemory(int shmid);

int getMsgQueueId(key_t msgKey, int flag);

void destroyMsgQueue(int msgQueueId);

void deattachSharedMemory(sharedState* state);

void destroySharedMemory(int shmid);

#endif