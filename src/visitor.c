#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>
#include "logger.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    srand(time(nullptr) + getpid());
    int age = rand() % 80 + 1;
    int routeVisited = 0; // 0 - none, 1 - route 1, 2 - route 2

    PRINT("I'm a visitor of age %d", age);

    key_t msgKey = generateKey(VISITOR_CASHIER_MSG);
    int msgQueueId = getMsgQueueId(msgKey, 0);

    TicketMessage msg;  // ticket request
    msg.mtype = 1;
    msg.age = age;
    msg.route = routeVisited;
    msg.isRepeat = 0;

    if (msgsnd(msgQueueId, &msg, sizeof(TicketMessage) - sizeof(long), 0) == -1) {
        PRINT_ERR("msgsnd");
        return 1;
    }

    return 0;
}