#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
    PRINT("I'm the guide!");

    int visitors[ROUTE_1_CAPACITY];

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGTERM, &signalHandler, nullptr);

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, 0);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(VISITOR_GUIDE_QUEUE_KEY_ID);
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
        while ( count < ROUTE_1_CAPACITY &&
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
                P(semId, GUIDE_BRIDGE_WAIT_SEM);
            }
            PRINT("%d visitors crossed the bridge to enter", count);

            if (!stop) {
                PRINT("Tour is in progress");
                sleep(ROUTE_1_DURATION);
                PRINT("Tour finished");
            }
            for (int i = 0; i < count; i++) {
                kill(visitors[i], SIGUSR1);
            }

            // wait for all visitors to exit and cross the bridge
            for (int i = 0; i < count; i++) {
                P(semId, GUIDE_BRIDGE_WAIT_SEM);
            }
            PRINT("%d visitors left the cave", count);
        }
    }

    PRINT("Finishing...");
    return 0;
}
