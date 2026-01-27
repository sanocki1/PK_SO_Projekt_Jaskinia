#include <signal.h>
#include <stdio.h>
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

    PRINT("I'm the guide!");

    int visitors[routeCapacity];

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGTERM, &signalHandler, NULL);

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, 0);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(visitorGuideQueueKeyId);
    int visitorGuideMsgQueueId = getMsgQueueId(key, 0);
    QueueMessage msg;

    key = generateKey(SEMAPHORE_KEY_ID);
    int semId = getSemaphoreId(key, SEM_COUNT, 0);

    // wait for some initial visitors to arrive so more than just the first group is processed
    sleep(VISITOR_FREQUENCY + 1);

    while (state->visitorCount || !stop) {
        int count = 0;
        int foundAdult = 0;
        pid_t visitorPid;

        // first loop to find at least one adult of any priority
        while (!foundAdult && !stop) {
            if (msgrcv(visitorGuideMsgQueueId, &msg,
                    sizeof(QueueMessage) - sizeof(long),
                    PRIORITY_HIGH_ADULT, IPC_NOWAIT) != -1
                || msgrcv(visitorGuideMsgQueueId, &msg,
                    sizeof(QueueMessage) - sizeof(long),
                    PRIORITY_NORMAL_ADULT, IPC_NOWAIT) != -1) {
                foundAdult = 1;
                visitorPid = msg.pid;
                if (stop) {
                    kill(visitorPid, SIGTERM);
                    continue;
                }
                visitors[count] = visitorPid;
                count++;
                kill(visitorPid, SIGUSR1);
            }
            else { usleep(100000); } // 100ms
        }

        // second loop to process the remaining people based on priority
        while ( count < routeCapacity &&
            msgrcv(visitorGuideMsgQueueId, &msg,
                sizeof(QueueMessage) - sizeof(long),
                -PRIORITY_NORMAL_ADULT, IPC_NOWAIT) != -1) {
            visitorPid = msg.pid;
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
            PRINT("Waiting for %d visitors to cross the bridge", count);

            // wait for all visitors in current group to cross the bridge
            for (int i = 0; i < count; i++) {
                P(semId, guideBridgeSem);
            }
            PRINT("%d visitors crossed the bridge to enter", count);

            if (!stop) {
                PRINT("Tour is in progress");
                sleep(routeDuration);
                PRINT("Tour finished");
            }
            for (int i = 0; i < count; i++) {
                kill(visitors[i], SIGUSR1);
            }

            // wait for all visitors to exit and cross the bridge
            for (int i = 0; i < count; i++) {
                P(semId, guideBridgeSem);
            }
            PRINT("%d visitors left the cave", count);
        }
    }

    PRINT("Finishing...");
    return 0;
}
