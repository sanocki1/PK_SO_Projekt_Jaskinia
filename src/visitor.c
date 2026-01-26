#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include "config.h"
#include "logger.h"
#include "utils.h"

volatile sig_atomic_t canProceed;
volatile sig_atomic_t rejected = 0;
void handleSignal(int sig) {
    if (sig == SIGUSR1) canProceed = 1;
    if (sig == SIGTERM) rejected = 1;
}

int main(int argc, char* argv[]) {
    pid_t pid = getpid();
    srand(time(nullptr) + pid);
    int age = rand() % 80 + 1;
    int wantsToVisit = 1;
    int isRepeat = 0;
    int routeToVisit = 0;
    canProceed = 0;

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGUSR1, &signalHandler, nullptr);
    sigaction(SIGTERM, &signalHandler, nullptr);

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, 0);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(VISITOR_CASHIER_QUEUE_KEY_ID);
    int visitorCashierMsgQueueId = getMsgQueueId(key, 0);

    key = generateKey(VISITOR_GUIDE_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId = getMsgQueueId(key, 0);

    key = generateKey(SEMAPHORE_KEY_ID);
    int semId = getSemaphoreId(key, SEM_COUNT, 0);

    P(semId, VISITOR_COUNT_SEM);
    state->visitorCount++;
    V(semId, VISITOR_COUNT_SEM);

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

        // send a message to guide that you want to enter
        QueueMessage queueMessage;
        if (age >= 8 && isRepeat) queueMessage.mtype = PRIORITY_HIGH_ADULT;
        else if (age < 8 && isRepeat) queueMessage.mtype = PRIORITY_HIGH_CHILD;
        else if (age >= 8 && !isRepeat) queueMessage.mtype = PRIORITY_NORMAL_ADULT;
        else queueMessage.mtype = PRIORITY_NORMAL_CHILD;
        queueMessage.pid = pid;
        if (msgsnd(visitorGuideMsgQueueId, &queueMessage,
            sizeof(QueueMessage) - sizeof(long), 0) == -1) {
            PRINT_ERR("msgsnd");
            return 1;
            }

        // wait for guide signal to enter the bridge/cave
        while (!canProceed && !rejected) {
            pause();
        }
        if (rejected) {
            break;
        }
        canProceed = 0;

        // bridge waiting semaphore
        P(semId, BRIDGE_SEM);
        sleep(BRIDGE_DURATION);
        V(semId, BRIDGE_SEM);

        //signal the guide that crossed the bridge
        V(semId, GUIDE_BRIDGE_WAIT_SEM);

        //wait for guide signal that the tour has ended
        while (!canProceed) {
            pause();
        }

        // bridge waiting semaphore to leave the cave
        P(semId, BRIDGE_SEM);
        sleep(BRIDGE_DURATION);
        V(semId, BRIDGE_SEM);

        // inform guide that you have crossed back
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