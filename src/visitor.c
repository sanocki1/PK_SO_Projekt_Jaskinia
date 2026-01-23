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
    int wantsToVisit = 1;
    int isRepeat = 0;
    int routeToVisit;

    PRINT("I'm a visitor of age %d", age);

    key_t visitorCashierMsgKey = generateKey(VISITOR_CASHIER_QUEUE);
    int visitorCashierMsgQueueId = getMsgQueueId(visitorCashierMsgKey, 0);

    key_t visitorGuideMsgKey = generateKey(VISITOR_GUIDE_QUEUE);
    int visitorGuideMsgQueueId = getMsgQueueId(visitorGuideMsgKey, 0);

    while (wantsToVisit) {
        if (age < 8 || age > 75) routeToVisit = 2;
        else if (isRepeat) routeToVisit = routeToVisit == 1 ? 2 : 1;
        else routeToVisit = rand() % 2 + 1;

        TicketMessage ticketMessage;
        ticketMessage.mtype = 1;
        ticketMessage.age = age;
        ticketMessage.route = routeToVisit;
        ticketMessage.isRepeat = isRepeat;

        if (msgsnd(visitorCashierMsgQueueId, &ticketMessage,
            sizeof(TicketMessage) - sizeof(long), 0) == -1) {
            PRINT_ERR("msgsnd");
            return 1;
            }

        QueueMessage queueMessage;
        queueMessage.mtype = isRepeat ? PRIORITY_REPEAT : PRIORITY_NORMAL;
        queueMessage.ppid = getpid();

        if (msgsnd(visitorGuideMsgQueueId, &queueMessage,
            sizeof(QueueMessage) - sizeof(long), 0) == -1) {
            PRINT_ERR("msgsnd");
            return 1;
            }

        //TODO: bridge waiting, route waiting, etc goes here

        if (rand() % 10 == 0) {
            wantsToVisit = 1;
            isRepeat = 1;
        } else {
            wantsToVisit = 0;
        }
    }
    return 0;
}