#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "logger.h"
#include <unistd.h>
#include <sys/msg.h>
#include "config.h"

int main(int argc, char* argv[]) {
    PRINT("I'm the guide!");

    key_t key = generateKey(VISITOR_GUIDE_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId = getMsgQueueId(key, 0);

    // initial idea to make sure adults are already on the route before processing children, create a separate mtype for
    // adults and initially only look for them, then switch to general prio queue after one is found, this ensures that
    // we get an adult, without blocking if there is N or more children at the front of the queue
    //alternative idea, create a semaphore that counts adults on route, and only allow children to proceed if there is at least one adult
    QueueMessage msg;
    int count = 0;
    while (
        count < ROUTE_1_CAPACITY &&
        msgrcv(visitorGuideMsgQueueId, &msg,
            sizeof(QueueMessage) - sizeof(long),
            -2, IPC_NOWAIT) != -1) {
        count++;
        PRINT("Received message from %d, is mtype: (1 - PRIORITY_REPEAT)%ld", msg.ppid, msg.mtype);
    }

    PRINT("Finishing...");
    return 0;
}
