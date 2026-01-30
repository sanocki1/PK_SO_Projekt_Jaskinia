#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SHM_KEY_ID 1
#define VISITOR_CASHIER_QUEUE_KEY_ID 2
#define SEMAPHORE_KEY_ID 3
#define VISITOR_GUIDE_QUEUE_KEY_ID_1 4
#define VISITOR_GUIDE_QUEUE_KEY_ID_2 5

// children before adults, so after 1 adult is found queue fills up with
// as many children as possible so even smaller chance of clogging the queue
#define PRIORITY_HIGH_CHILD 1
#define PRIORITY_HIGH_ADULT 2
#define PRIORITY_NORMAL_CHILD 3
#define PRIORITY_NORMAL_ADULT 4

#define SEM_COUNT 9
#define BRIDGE_SEM_1 0
#define BRIDGE_SEM_2 1
#define VISITOR_COUNT_SEM 2
#define GUIDE_BRIDGE_SEM_1 3
#define GUIDE_BRIDGE_SEM_2 4
#define LOG_SEM 5
#define QUEUE_SEM_1 6
#define QUEUE_SEM_2 7
#define TICKET_QUEUE_SEM 8

#define EINTR 4
#define MAIN_PROCESSES_COUNT 5 // main, cashier, guard, 2 guides
#define TICKET_MESSAGE_SIZE (sizeof(int) * 2)
#define QUEUE_MESSAGE_SIZE sizeof(int)

/** @brief Pamięc wspóldzielona */
typedef struct {
    int Tp; // opening time
    int Tk; // closing time
    int closing; // 0 - simulation running, 1 - simulation closing
    int ticketsSold;
    double moneyEarned;
    int visitorCount;
} sharedState;

/** @brief Komunikat wysłany do kasjera */
typedef struct {
    long mtype;
    int age;
    int isRepeat;
} TicketMessage;

/** @brief Komunikat wysłany do przewodnika */
typedef struct {
    long mtype;
    int pid;
} QueueMessage;

key_t generateKey(int id);

int getShmid(key_t shmKey, int flag);
int createSharedMemory(int keyId);
int openSharedMemory(int keyId);
sharedState* attachSharedMemory(int shmid);
void deattachSharedMemory(sharedState* state);
void destroySharedMemory(int shmid);

int getMsgQueueId(key_t msgKey, int flag);
int createMsgQueue(int keyId);
int openMsgQueue(int keyId);
void destroyMsgQueue(int msgQueueId);

int getSemaphoreId(key_t semKey,int count, int flag);
int createSemaphore(int keyId, int count);
int openSemaphore(int keyId, int count);
void initializeSemaphore(int semId, int semNum ,int initialValue);
void destroySemaphore(int semId);

void P(int semid, int semnum);
void V(int semid, int semnum);

#endif