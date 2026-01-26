#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>

#include "config.h"
#include "logger.h"
#include "utils.h"

volatile sig_atomic_t canProceed = 0;
void handleSignal(int sig) {
    if (sig == SIGUSR1) canProceed = 1;
}

int main(int argc, char* argv[]) {
    pid_t pid = getpid();
    srand(time(nullptr) + pid);
    int age = rand() % 80 + 1;
    int wantsToVisit = 1;
    int isRepeat = 0;
    int routeToVisit = 0;

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGUSR1, &signalHandler, nullptr);

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, 0);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(VISITOR_CASHIER_QUEUE_KEY_ID);
    int visitorCashierMsgQueueId = getMsgQueueId(key, 0);

    key = generateKey(VISITOR_GUIDE_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId = getMsgQueueId(key, 0);

    key = generateKey(SEMAPHORE_KEY_ID);
    int semId = getSemaphoreId(key, SEM_COUNT, 0);

    PRINT("%d entering, visitor count before entering = %d", pid, state->visitorCount);
    P(semId, VISITOR_COUNT_SEM);
    state->visitorCount++;
    V(semId, VISITOR_COUNT_SEM);
    PRINT("%d entered, visitor count after entering = %d", pid, state->visitorCount);

    while (!state->closing && wantsToVisit) {
        if (age < 8 || age > 75) routeToVisit = 2;
        else if (isRepeat) routeToVisit = routeToVisit == 1 ? 2 : 1;
        else routeToVisit = rand() % 2 + 1;

        TicketMessage ticketMessage;
        ticketMessage.mtype = 1;
        ticketMessage.age = age;
        ticketMessage.isRepeat = isRepeat;
        if (msgsnd(visitorCashierMsgQueueId, &ticketMessage,
            sizeof(TicketMessage) - sizeof(long), 0) == -1) {
            PRINT_ERR("msgsnd");
            return 1;
            }

        QueueMessage queueMessage;
        queueMessage.mtype = isRepeat ? PRIORITY_HIGH : PRIORITY_NORMAL;
        queueMessage.pid = pid;

        // send a message to guide that you want to enter
        if (msgsnd(visitorGuideMsgQueueId, &queueMessage,
            sizeof(QueueMessage) - sizeof(long), 0) == -1) {
            PRINT_ERR("msgsnd");
            return 1;
            }

        // wait for guide to signal you can proceed
        while (!canProceed) {
            sleep(1);
        }

        // bridge waiting semaphore
        P(semId, BRIDGE_SEM);
        sleep(BRIDGE_DURATION);
        V(semId, BRIDGE_SEM);

        //signal the guide that crossed the bridge
        V(semId, GUIDE_BRIDGE_WAIT_SEM);


        if (!isRepeat && rand() % 10 == 0) {
            isRepeat = 1;
        } else {
            wantsToVisit = 0;
        }
    }

    P(semId, VISITOR_COUNT_SEM);
    state->visitorCount--;
    V(semId, VISITOR_COUNT_SEM);
    PRINT("%d leaving, visitor count after leaving = %d", pid, state->visitorCount);

    deattachSharedMemory(state);

    return 0;
}