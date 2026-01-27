/**
* @file guide.c
* @brief Proces przewodnika.
*
* Proces:
* - obsługuje jedną z tras
* - pobiera odwiedzających z kolejki komunikatów
* - dba o obecność co najmniej jednego dorosłego w grupie
* - synchronizuje przejścia przez most za pomocą semaforów
* - prowadzi zwiedzanie trasy i wypuszcza grupę po zakończeniu.
*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "logger.h"
#include <unistd.h>
#include <sys/msg.h>
#include "config.h"


volatile sig_atomic_t stop = 0;
void handleSignal(int sig) {
    if (sig == SIGTERM) stop = 1;
}
/** @brief Odbiera komunikat z kolejki. */
int receiveMessage(int queueId, QueueMessage* msg, long msgType);

/** @brief Wyszukuje dorosłego zwiedzającego w kolejce. */
int findAdult(int queueId, QueueMessage* msg);

/** @brief Wysyła sygnał do pojedynczego zwiedzającego. */
void signalVisitor(pid_t visitorPid, int signal);

/** @brief Wysyła sygnał do grupy zwiedzających. */
void signalVisitors(pid_t* visitors, int count, int signal);

/** @brief Oczekuje na przejście grupy przez most. */
void waitForBridgeCrossing(int semId, int bridgeSem, int count);

int main(int argc, char* argv[]) {
    int routeCapacity;
    int visitorGuideQueueKeyId;
    int guideBridgeSem;
    int routeDuration;
    if (strcmp(argv[1], "1") == 0) {    // guide 1
        routeCapacity = ROUTE_1_CAPACITY;
        visitorGuideQueueKeyId = VISITOR_GUIDE_QUEUE_KEY_ID_1;
        guideBridgeSem = GUIDE_BRIDGE_SEM_1;
        routeDuration = ROUTE_1_DURATION;
    }
    else {                              //guide 2
        routeCapacity = ROUTE_2_CAPACITY;
        visitorGuideQueueKeyId = VISITOR_GUIDE_QUEUE_KEY_ID_2;
        guideBridgeSem = GUIDE_BRIDGE_SEM_2;
        routeDuration = ROUTE_2_DURATION;
    }
    int visitors[routeCapacity];

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    if (sigaction(SIGTERM, &signalHandler, NULL) == -1) {
        perror("sigaction SIGTERM");
        return EXIT_FAILURE;
    }

    int shmid = openSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int visitorGuideMsgQueueId = openMsgQueue(visitorGuideQueueKeyId);
    int semId = openSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initLogger(semId);
    LOG("I'm the guide on route, %s!", argv[1]);

    // wait for some initial visitors to arrive so more than just the first group is processed
    sleep(VISITOR_FREQUENCY + 1);
    sleep(60); //TODO THIS IS A TEMPORARY SLEEP FOR TESTING
    while (state->visitorCount || !stop) {
        int count = 0;
        int foundAdult = 0;
        QueueMessage msg;

        // first loop to find at least one adult of any priority
        while (!foundAdult && !stop) {
            if (findAdult(visitorGuideMsgQueueId, &msg)) {
                foundAdult = 1;
                pid_t visitorPid = msg.pid;
                if (stop) {
                    kill(visitorPid, SIGTERM);
                    continue;
                }
                visitors[count] = visitorPid;
                count++;
                signalVisitor(visitorPid, SIGUSR1);
            }
            else { usleep(100000); } // 100ms
        }

        // second loop to process the remaining people based on priority
        while ( count < routeCapacity &&
                receiveMessage(visitorGuideMsgQueueId, &msg, -PRIORITY_NORMAL_ADULT)) {
            pid_t visitorPid = msg.pid;
            // tour is closed, reject the visitor from the queue
            if (stop) {
                kill(visitorPid, SIGTERM);
                continue;
            }
            visitors[count] = visitorPid;
            count++;
            //signal visitor to continue
            kill(visitorPid, SIGUSR1);
                }

        if (count > 0) {
            LOG("Waiting for %d visitors to cross the bridge to enter.", count);
            waitForBridgeCrossing(semId, guideBridgeSem, count);
            LOG("%d visitors crossed the bridge to enter.", count);

            if (!stop) {
                LOG("Tour is in progress.");
                sleep(routeDuration);
                LOG("Tour finished.");
            }

            signalVisitors(visitors, count, SIGUSR1);

            LOG("Waiting for %d visitors to cross the bridge to leave.", count);
            waitForBridgeCrossing(semId, guideBridgeSem, count);
            LOG("%d visitors left the cave.", count);
        }
    }

    deattachSharedMemory(state);
    LOG("Finishing...");
    return 0;
}

int receiveMessage(int queueId, QueueMessage* msg, long msgType) {
    return msgrcv(queueId, msg, sizeof(QueueMessage) - sizeof(long),
                  msgType, IPC_NOWAIT) != -1;
}

int findAdult(int queueId, QueueMessage* msg) {
    return receiveMessage(queueId, msg, PRIORITY_HIGH_ADULT) ||
           receiveMessage(queueId, msg, PRIORITY_NORMAL_ADULT);
}

void waitForBridgeCrossing(int semId, int bridgeSem, int count) {
    for (int i = 0; i < count; i++) {
        P(semId, bridgeSem);
    }
}

void signalVisitor(pid_t visitorPid, int signal) {
    if (kill(visitorPid, signal) == -1) {
        perror("kill visitor");
    }
}

void signalVisitors(pid_t* visitors, int count, int signal) {
    for (int i = 0; i < count; i++) {
        signalVisitor(visitors[i], signal);
    }
}