#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include "config.h"
#include "logger.h"
#include "utils.h"

volatile sig_atomic_t canProceed;
volatile sig_atomic_t rejected = 0;
void handleSignal(int sig) {
    if (sig == SIGUSR1) canProceed = 1;
    if (sig == SIGTERM) rejected = 1;
}

int selectRoute(int age, int isRepeat, int previousRoute);

long getQueuePriority(int age, int isRepeat);

void buyTicket(int queueId, int age, int isRepeat);

void joinQueue(int queueId, pid_t pid, long priority);

void waitForSignal(void);

void crossBridge(int semId, int bridgeSem);

void updateVisitorCount(sharedState* state, int semId, int delta);


int main(int argc, char* argv[]) {
    pid_t pid = getpid();
    srand(time(NULL) + pid);
    int age = rand() % 80 + 1;
    int wantsToVisit = 1;
    int isRepeat = 0;
    int routeToVisit = 0;

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    if (sigaction(SIGUSR1, &signalHandler, NULL) == -1) {
        perror("sigaction SIGUSR1");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGTERM, &signalHandler, NULL) == -1) {
        perror("sigaction SIGTERM");
        return EXIT_FAILURE;
    }

    int shmid = openSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int visitorCashierMsgQueueId = openMsgQueue(VISITOR_CASHIER_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId1 = openMsgQueue(VISITOR_GUIDE_QUEUE_KEY_ID_1);
    int visitorGuideMsgQueueId2 = openMsgQueue(VISITOR_GUIDE_QUEUE_KEY_ID_2);
    int semId = openSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initLogger(semId);

    LOG("I'm a visitor!");

    updateVisitorCount(state, semId, 1);

    while (!state->closing && wantsToVisit) {
        routeToVisit = selectRoute(age, isRepeat, routeToVisit);
        canProceed = 0;

        LOG("Buying a ticket for route %d.", routeToVisit);
        buyTicket(visitorCashierMsgQueueId, age, isRepeat);

        long priority = getQueuePriority(age, isRepeat);
        int queueId = routeToVisit == 1 ? visitorGuideMsgQueueId1 : visitorGuideMsgQueueId2;
        LOG("Joining queue %d with priority %ld.", routeToVisit, priority);
        joinQueue(queueId, pid, priority);

        // wait for guide signal to enter the bridge/cave
        waitForSignal();
        if (rejected) break;
        canProceed = 0;

        int bridgeSemaphore = routeToVisit == 1 ? BRIDGE_SEM_1 : BRIDGE_SEM_2;
        int guideBridgeSemaphore = routeToVisit == 1 ? GUIDE_BRIDGE_SEM_1 : GUIDE_BRIDGE_SEM_2;

        LOG("Waiting to cross the bridge to route %d", routeToVisit);
        // wait for bridge capacity to enter, then signal guide
        crossBridge(semId, bridgeSemaphore);
        V(semId, guideBridgeSemaphore);
        LOG("Crossed the bridge.");

        //wait for guide signal that the tour has ended
        waitForSignal();
        LOG("Tour ended.");

        LOG("Waiting to cross the bridge back.");
        // wait for bridge capacity to leave, then signal guide
        crossBridge(semId, bridgeSemaphore);
        V(semId, guideBridgeSemaphore);
        LOG("Crossed the bridge back.");


        if (!isRepeat && rand() % 10 == 0) {
            isRepeat = 1;
            LOG("Decided to visit again.");
        } else {
            wantsToVisit = 0;
            LOG("No more visits.");
        }
    }

    updateVisitorCount(state, semId, -1);
    LOG("Leaving...");

    deattachSharedMemory(state);
    return 0;
}


int selectRoute(int age, int isRepeat, int previousRoute) {
    if (age < 8 || age > 75) return 2;
    if (isRepeat) return previousRoute == 1 ? 2 : 1;
    return rand() % 2 + 1;
}

long getQueuePriority(int age, int isRepeat) {
    if (age >= 8 && isRepeat) return PRIORITY_HIGH_ADULT;
    if (age < 8 && isRepeat) return PRIORITY_HIGH_CHILD;
    if (age >= 8 && !isRepeat) return PRIORITY_NORMAL_ADULT;
    return PRIORITY_NORMAL_CHILD;
}

void buyTicket(int queueId, int age, int isRepeat) {
    TicketMessage msg;
    msg.mtype = 1;
    msg.age = age;
    msg.isRepeat = isRepeat;
    if (msgsnd(queueId, &msg, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
        perror("msgsnd ticket");
        exit(EXIT_FAILURE);
    }
}

void joinQueue(int queueId, pid_t pid, long priority) {
    QueueMessage msg;
    msg.mtype = priority;
    msg.pid = pid;
    if (msgsnd(queueId, &msg, sizeof(QueueMessage) - sizeof(long), 0) == -1) {
        perror("msgsnd queue");
        exit(EXIT_FAILURE);
    }
}

void waitForSignal(void) {
    while (!canProceed && !rejected) {
        pause();
    }
}

void crossBridge(int semId, int bridgeSem) {
    P(semId, bridgeSem);
    sleep(BRIDGE_DURATION);
    V(semId, bridgeSem);
}

void updateVisitorCount(sharedState* state, int semId, int delta) {
    P(semId, VISITOR_COUNT_SEM);
    state->visitorCount += delta;
    V(semId, VISITOR_COUNT_SEM);
}