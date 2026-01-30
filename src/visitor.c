/**
* @file visitor.c
* @brief Visitor process.
*
* Process:
* - randomizes its age
* - chooses a route
* - joins a queue to the path
* - buys a ticket
* - crosses a bridge, waits for the tour to end, crosses the bridge back
* - optionally repeats the tour
*/

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

/** @brief Handles signals to continue/stop the tour receives from the guide. */
void handleSignal(int sig);

/** @brief Chooses a route based on age and past tours. */
int selectRoute(int age, int isRepeat, int previousRoute);

/** @brief Returns a queue priority based on age and whether it's a repeat visit. */
long getQueuePriority(int age, int isRepeat);

/** @brief Sends a message to the cashier with information required for ticket pricing. */
void buyTicket(int queueId, int age, int isRepeat, int semId);

/** @brief Joins a cave entrance queue. */
void joinQueue(int queueId, pid_t pid, long priority, int semId, int queueSem);

/** @brief Waits for a signal from the guide. */
void waitForSignal(void);

/** @brief Crosses the bridge, waiting for bridge semaphore. */
void crossBridge(int semId, int bridgeSem);

/** @biref Updates a global visitor count. */
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

        long priority = getQueuePriority(age, isRepeat);
        int queueId = routeToVisit == 1 ? visitorGuideMsgQueueId1 : visitorGuideMsgQueueId2;
        int queueSem = routeToVisit == 1 ? QUEUE_SEM_1 : QUEUE_SEM_2;
        LOG("Joining queue %d with priority %ld.", routeToVisit, priority);
        joinQueue(queueId, pid, priority, semId, queueSem);

        // wait for a guide signal to enter the bridge/cave
        waitForSignal();
        if (rejected) break;
        canProceed = 0;

        LOG("Buying a ticket for route %d.", routeToVisit);
        buyTicket(visitorCashierMsgQueueId, age, isRepeat, semId);

        int bridgeSemaphore = routeToVisit == 1 ? BRIDGE_SEM_1 : BRIDGE_SEM_2;
        int guideBridgeSemaphore = routeToVisit == 1 ? GUIDE_BRIDGE_SEM_1 : GUIDE_BRIDGE_SEM_2;

        LOG("Waiting to cross the bridge to route %d", routeToVisit);
        // wait for bridge capacity to enter, then signal guide
        crossBridge(semId, bridgeSemaphore);
        V(semId, guideBridgeSemaphore);
        LOG("Crossed the bridge.");

        //wait for guide signal that the tour has ended
        LOG("Starting the tour.");
        waitForSignal();
        LOG("Tour ended.");

        LOG("Waiting to cross the bridge back.");
        // wait for bridge capacity to leave, then signal the guide
        crossBridge(semId, bridgeSemaphore);
        V(semId, guideBridgeSemaphore);
        LOG("Crossed the bridge back.");


        if (!state->closing && !isRepeat && rand() % 10 == 0) {
            isRepeat = 1;
            LOG("Decided to visit again.");
        } else {
            wantsToVisit = 0;
            LOG("No more visits.");
        }
    }

    updateVisitorCount(state, semId, -1);
    deattachSharedMemory(state);

    LOG("Leaving...");
    return 0;
}

void handleSignal(int sig) {
    if (sig == SIGUSR1) canProceed = 1;
    if (sig == SIGTERM) rejected = 1;
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

void buyTicket(int queueId, int age, int isRepeat, int semId) {
    TicketMessage msg;
    msg.mtype = 1;
    msg.age = age;
    msg.isRepeat = isRepeat;
    P(semId, TICKET_QUEUE_SEM);
    if (msgsnd(queueId, &msg, TICKET_MESSAGE_SIZE, 0) == -1) {
        perror("msgsnd ticket");
        exit(EXIT_FAILURE);
    }
}

void joinQueue(int queueId, pid_t pid, long priority, int semId, int queueSem) {
    QueueMessage msg;
    msg.mtype = priority;
    msg.pid = pid;
    P(semId, queueSem);
    if (msgsnd(queueId, &msg, QUEUE_MESSAGE_SIZE, 0) == -1) {
        perror("msgsnd queue");
        V(semId, queueSem);
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