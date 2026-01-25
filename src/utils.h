#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SHM_KEY_ID 1
#define VISITOR_CASHIER_QUEUE_KEY_ID 2
#define SEMAPHORE_KEY_ID 3
#define VISITOR_GUIDE_QUEUE_KEY_ID 4

#define PRIORITY_REPEAT 1
#define PRIORITY_NORMAL 2

#define SEM_COUNT 1
#define CASHIER_SEM 0
#define ADULT_IN_QUEUE_SEM 1

#define EINTR 4

typedef struct {
    int Tp; // opening time
    int Tk; // closing time
    int closing; // 0 - simulation running, 1 - simulation closing
    int adultsOnRouteOneAmount;
    int adultsOnRouteTwoAmount;
    int ticketsSold;
    double moneyEarned;
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

sharedState* attachSharedMemory(int shmid);

void deattachSharedMemory(sharedState* state);

void destroySharedMemory(int shmid);

int getMsgQueueId(key_t msgKey, int flag);

void destroyMsgQueue(int msgQueueId);

int getSemaphoreId(key_t semKey,int count, int flag);

void initializeSemaphore(int semId, int semNum ,int initialValue);

void destroySemaphore(int semId);

void P(int semid, int semnum);

void V(int semid, int semnum);

#endif