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
    if (sig == SIGINT) stop = 1;
}

int main(int argc, char* argv[]) {
    PRINT("I'm the guide!");

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGINT, &signalHandler, nullptr);

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, 0);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(VISITOR_GUIDE_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId = getMsgQueueId(key, 0);
    QueueMessage msg;

    key = generateKey(SEMAPHORE_KEY_ID);
    int semId = getSemaphoreId(key, SEM_COUNT, 0);
    // initial idea to make sure adults are already on the route before processing children, create a separate mtype for
    // adults and initially only look for them, then switch to general prio queue after one is found, this ensures that
    // we get an adult, without blocking if there is N or more children at the front of the queue
    //alternative idea, create a semaphore that counts adults on route, and only allow children to proceed if there is at least one adult

    while (state->visitorCount || !stop) {
        int count = 0;
        while ( count < ROUTE_1_CAPACITY &&
            msgrcv(visitorGuideMsgQueueId, &msg,
                sizeof(QueueMessage) - sizeof(long),
                -2, IPC_NOWAIT) != -1) {
            count++;
            pid_t visitorPid = msg.pid;
            //signal visitor to continue
            kill(visitorPid, SIGUSR1);
            PRINT("Sent signal to %d", visitorPid);
                }
        if (count > 0) {
            PRINT("Waiting for %d visitors to cross the bridge", count);

            //wait for all visitors in current group to cross the bridge
            for (int i = 0; i < count; i++) {
                P(semId, GUIDE_BRIDGE_WAIT_SEM);
            }
            PRINT("%d visitors crossed the bridge", count);
            PRINT("TOUR IS NOW IN PROGRESS");
            sleep(5);
        }
    }

    PRINT("Finishing...");
    return 0;
}
