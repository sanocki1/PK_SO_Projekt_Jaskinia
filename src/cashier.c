#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include "logger.h"
#include "config.h"
#include "utils.h"

static sharedState* state;

void handleSigint(int sig) {
    PRINT("Caught SIGINT, terminating cashier...");
    deattachSharedMemory(state);
    exit(0);
}

int main(int argc, char* argv[]) {
    PRINT("I'm the cashier!");

    struct sigaction sigIntHandler = {0};
    sigIntHandler.sa_handler = handleSigint;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    key_t shmKey = generateKey(SHM_KEY);
    int shmid = getShmid(shmKey, 0);
    state = getSharedMemory(shmid);

    key_t visitorCashierMsgKey = generateKey(VISITOR_CASHIER_QUEUE);
    int visitorCashierMsgQueueId = getMsgQueueId(visitorCashierMsgKey, 0);

    while (1) {
        TicketMessage msg;
        if (msgrcv(visitorCashierMsgQueueId, &msg, sizeof(TicketMessage) - sizeof(long), 0, 0) == -1) {
            PRINT_ERR("msgrcv");
            continue;
        }
        PRINT("Visitor age %d, route %d", msg.age, msg.route);
    }

    PRINT("Finishing...");
}
