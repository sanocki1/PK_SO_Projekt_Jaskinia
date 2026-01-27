#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include "config.h"
#include "logger.h"
#include "utils.h"

int validateParameters();

ulong getMaxVisitorCount();

void initializeSharedState(sharedState*);

void initializeSemaphores(int semId);

pid_t spawnProcess(const char* executable, char* const args[]);

void spawnVisitorGroup(int groupSize);

void cleanupResources(int visitorCashierMsgQueueId, int visitorGuideMsgQueueId1,
                            int visitorGuideMsgQueueId2, sharedState* state,
                            int shmid, int semId);


int main(int argc, char* argv[]) {
    clearLog();
    
    if (!validateParameters()) {
        return 1;
    }

    ulong maxVisitorCount = getMaxVisitorCount();
    if (maxVisitorCount == 0) {
        return 1;
    }

    srand(time(NULL) + getpid());

    int shmid = createSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int visitorCashierMsgQueueId = createMsgQueue(VISITOR_CASHIER_QUEUE_KEY_ID);
    int visitorGuideMsgQueueId1 = createMsgQueue(VISITOR_GUIDE_QUEUE_KEY_ID_1);
    int visitorGuideMsgQueueId2 = createMsgQueue(VISITOR_GUIDE_QUEUE_KEY_ID_2);

    int semId = createSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initializeSemaphores(semId);
    initializeSharedState(state);

    initLogger(semId);
    LOG("I'm the main process!");
    LOG("Startup parameters have been initialized.");

    char cashierPidStr[16], guide1PidStr[16], guide2PidStr[16];
    char* cashierArgs[] = {"cashier", NULL};
    char* guide1Args[] = {"guide", "1", NULL};
    char* guide2Args[] = {"guide", "2", NULL};

    pid_t cashierPid = spawnProcess("./cashier", cashierArgs);
    snprintf(cashierPidStr, sizeof(cashierPidStr), "%d", cashierPid);

    pid_t guide1Pid = spawnProcess("./guide", guide1Args);
    snprintf(guide1PidStr, sizeof(guide1PidStr), "%d", guide1Pid);

    pid_t guide2Pid = spawnProcess("./guide", guide2Args);
    snprintf(guide2PidStr, sizeof(guide2PidStr), "%d", guide2Pid);

    char* guardArgs[] = {"guard", cashierPidStr, guide1PidStr, guide2PidStr, NULL};
    spawnProcess("./guard", guardArgs);

    LOG("Simulation started. Visitors are arriving...");

    while (!state->closing) {
        int groupCount = rand() % MAX_VISITOR_GROUP_SIZE + 1;
        if (state->visitorCount + groupCount <= maxVisitorCount) {
            LOG("Visitor group of size %d arrived", groupCount);
            spawnVisitorGroup(groupCount);
        }
        sleep(VISITOR_FREQUENCY);
    }

    while (wait(NULL) > 0) {}

    LOG("Tickets sold for the day: %d", state->ticketsSold);
    LOG("Total money earned: %.2f", state->moneyEarned);
    LOG("Closing the simulation.");

    destroyMsgQueue(visitorCashierMsgQueueId);
    destroyMsgQueue(visitorGuideMsgQueueId1);
    destroyMsgQueue(visitorGuideMsgQueueId2);
    deattachSharedMemory(state);
    destroySharedMemory(shmid);
    destroySemaphore(semId);

    return 0;
}


int validateParameters() {
    if (OPENING_TIME < 0 || CLOSING_TIME > 24 || OPENING_TIME >= CLOSING_TIME || SECONDS_PER_HOUR < 1 ||
        SECONDS_PER_HOUR <= 0 || VISITOR_FREQUENCY <= 0) {
        LOG_ERR("Invalid time parameters");
        return 0;
    }
    if (ROUTE_1_CAPACITY <= BRIDGE_CAPACITY || ROUTE_2_CAPACITY <= BRIDGE_CAPACITY || BRIDGE_CAPACITY <= 0) {
        LOG_ERR("Invalid capacity parameters");
        return 0;
    }
    if (ROUTE_1_DURATION <= 0 || ROUTE_2_DURATION <= 0 || BRIDGE_DURATION <= 0) {
        LOG_ERR("Invalid duration parameters");
        return 0;
    }
    if (MAX_VISITOR_GROUP_SIZE <= 0) {
        LOG_ERR("Invalid visitor group size parameters");
        return 0;
    }
    if (BASE_TICKET_PRICE <= 0) {
        LOG_ERR("Invalid ticket price parameters");
        return 0;
    }
    return 1;
}

ulong getMaxVisitorCount() {
    struct rlimit limit;
    if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
        LOG_ERR("getrlimit");
        return 0;
    }
    if (limit.rlim_cur < (MAIN_PROCESSES_COUNT + MAX_VISITOR_GROUP_SIZE)) {
        LOG_ERR("Not enough process limit to run the simulation");
        return 0;
    }
    return limit.rlim_cur - MAIN_PROCESSES_COUNT;
}

void initializeSharedState(sharedState* state) {
    state->Tp = OPENING_TIME;
    state->Tk = CLOSING_TIME;
    state->closing = 0;
    state->moneyEarned = 0;
    state->ticketsSold = 0;
    state->visitorCount = 0;
}

void initializeSemaphores(int semId) {
    initializeSemaphore(semId, BRIDGE_SEM_1, BRIDGE_CAPACITY);
    initializeSemaphore(semId, BRIDGE_SEM_2, BRIDGE_CAPACITY);
    initializeSemaphore(semId, VISITOR_COUNT_SEM, 1);
    initializeSemaphore(semId, GUIDE_BRIDGE_SEM_1, 0);
    initializeSemaphore(semId, GUIDE_BRIDGE_SEM_2, 0);
    initializeSemaphore(semId, LOG_SEM, 1);
}

pid_t spawnProcess(const char* executable, char* const args[]) {
    pid_t pid = fork();
    if (pid == -1) {
        LOG_ERR("%s fork", executable);
        return -1;
    }
    if (pid == 0) {
        execl(executable, args[0], args[1], args[2], args[3], args[4], NULL);
        LOG_ERR("%s execl failed", executable);
        exit(1);
    }
    return pid;
}

void spawnVisitorGroup(int groupSize) {
    for (int i = 0; i < groupSize; i++) {
        char* args[] = {"visitor", NULL};
        spawnProcess("./visitor", args);
    }
}

void cleanupResources(int visitorCashierMsgQueueId, int visitorGuideMsgQueueId1,
                            int visitorGuideMsgQueueId2, sharedState* state,
                            int shmid, int semId) {
    destroyMsgQueue(visitorCashierMsgQueueId);
    destroyMsgQueue(visitorGuideMsgQueueId1);
    destroyMsgQueue(visitorGuideMsgQueueId2);
    deattachSharedMemory(state);
    destroySharedMemory(shmid);
    destroySemaphore(semId);
}