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

    key_t visitorGuideMsgKey = generateKey(VISITOR_GUIDE_QUEUE);
    int visitorGuideMsgQueueId = getMsgQueueId(visitorGuideMsgKey, 0);

    // sleep(25);

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
