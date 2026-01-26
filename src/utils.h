#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SHM_KEY_ID 1
#define VISITOR_CASHIER_QUEUE_KEY_ID 2
#define SEMAPHORE_KEY_ID 3
#define VISITOR_GUIDE_QUEUE_KEY_ID 4

// children before adults, so after 1 adult is found queue fills up with
// as many children as possible so even less chance of cloging the queue
#define PRIORITY_HIGH_CHILD 1
#define PRIORITY_HIGH_ADULT 2
#define PRIORITY_NORMAL_CHILD 3
#define PRIORITY_NORMAL_ADULT 4

#define SEM_COUNT 3
#define BRIDGE_SEM 0
#define VISITOR_COUNT_SEM 1
#define GUIDE_BRIDGE_WAIT_SEM 2

#define EINTR 4

typedef struct {
    int Tp; // opening time
    int Tk; // closing time
    int closing; // 0 - simulation running, 1 - simulation closing
    int ticketsSold;
    double moneyEarned;
    int visitorCount;
} sharedState;

typedef struct {
    long mtype;
    int age;
    int isRepeat;
} TicketMessage;

typedef struct {
    long mtype;
    int pid;
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