/**
* @file main.c
* @brief Główny proces symulacji jaskini.
*
* Odpowiada za:
* - inicjalizację pamięci współdzielonej, semaforów i kolejek komunikatów
* - uruchomienie procesów: kasjera, strażnika, przewodników i zwiedzających
* - generowanie grup zwiedzających
* - sprzątanie zasobów IPC po zakończeniu symulacji
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include "config.h"
#include "logger.h"
#include "utils.h"

/** @brief Sprawdza poprawność parametrów symulacji. */
int validateParameters();

/** @brief Oblicza maksymalną liczbę odwiedzających możliwą do obsługi. */
ulong getMaxVisitorCount();

/** @brief Inicjalizuje pamięć współdzieloną. */
void initializeSharedState(sharedState*);

/** @brief Inicjalizuje semafory. */
void initializeSemaphores(int semId);

/** @brief Tworzy nowy proces i uruchamia wskazany program. */
pid_t spawnProcess(const char* executable, char* const args[]);

/** @brief Tworzy wskazaną ilośc procesów zwiedzających */
void spawnVisitorGroup(int groupSize);

/** @brief Czyści dane z IPC */
void cleanupResources();

void handleSignal(int sig);

static int visitorCashierMsgQueueId;
static int visitorGuideMsgQueueId1;
static int visitorGuideMsgQueueId2;
static sharedState* state;
static int shmid;
static int semId;


int main(int argc, char* argv[]) {
    clearLog();
    
    if (!validateParameters()) {
        return EXIT_FAILURE;
    }

    ulong maxVisitorCount = getMaxVisitorCount();
    if (maxVisitorCount == 0) {
        return EXIT_FAILURE;
    }

    srand(time(NULL) + getpid());

    shmid = createSharedMemory(SHM_KEY_ID);
    state = attachSharedMemory(shmid);

    visitorCashierMsgQueueId = createMsgQueue(VISITOR_CASHIER_QUEUE_KEY_ID);
    visitorGuideMsgQueueId1 = createMsgQueue(VISITOR_GUIDE_QUEUE_KEY_ID_1);
    visitorGuideMsgQueueId2 = createMsgQueue(VISITOR_GUIDE_QUEUE_KEY_ID_2);

    semId = createSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initializeSemaphores(semId);
    initializeSharedState(state);

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    if (sigaction(SIGINT, &signalHandler, NULL) == -1) {
        perror("sigaction INT");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGTERM, &signalHandler, NULL) == -1) {
        perror("sigaction SIGTERM");
        return EXIT_FAILURE;
    }

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

    int licznik = 5000;
    while (licznik--) {
        // while (!state->closing) {
        // int groupCount = rand() % MAX_VISITOR_GROUP_SIZE + 1;
        int groupCount = 1;
        if (state->visitorCount + groupCount <= maxVisitorCount) {
            LOG("Visitor group of size %d arrived", groupCount);
            spawnVisitorGroup(groupCount);
        }
        // sleep(VISITOR_FREQUENCY);
        // }
    }

    while (wait(NULL) > 0) {}

    LOG("Tickets sold for the day: %d", state->ticketsSold);
    LOG("Total money earned: %.2f", state->moneyEarned);
    LOG("Closing the simulation.");

    cleanupResources();

    return 0;
}


int validateParameters() {
    if (OPENING_TIME < 0 || CLOSING_TIME > 24 || OPENING_TIME >= CLOSING_TIME || SECONDS_PER_HOUR < 1 ||
        SECONDS_PER_HOUR <= 0 || VISITOR_FREQUENCY <= 0) {
        perror("Invalid time parameters");
        return 0;
    }
    if (ROUTE_1_CAPACITY <= BRIDGE_CAPACITY || ROUTE_2_CAPACITY <= BRIDGE_CAPACITY || BRIDGE_CAPACITY <= 0) {
        perror("Invalid capacity parameters");
        return 0;
    }
    if (ROUTE_1_DURATION <= 0 || ROUTE_2_DURATION <= 0 || BRIDGE_DURATION <= 0) {
        perror("Invalid duration parameters");
        return 0;
    }
    if (MAX_VISITOR_GROUP_SIZE <= 0) {
        perror("Invalid visitor group size parameters");
        return 0;
    }
    if (BASE_TICKET_PRICE <= 0) {
        perror("Invalid ticket price parameters");
        return 0;
    }
    return 1;
}

ulong getMaxVisitorCount() {
    struct rlimit limit;
    if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
        perror("getrlimit");
        return 0;
    }
    if (limit.rlim_cur < (MAIN_PROCESSES_COUNT + MAX_VISITOR_GROUP_SIZE)) {
        perror("Not enough process limit to run the simulation");
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
        perror("fork");
        cleanupResources(visitorCashierMsgQueueId, visitorGuideMsgQueueId1,
                        visitorGuideMsgQueueId2, state, shmid, semId);
        return -1;
    }
    if (pid == 0) {
        execl(executable, args[0], args[1], args[2], args[3], args[4], NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }
    return pid;
}

void spawnVisitorGroup(int groupSize) {
    for (int i = 0; i < groupSize; i++) {
        char* args[] = {"visitor", NULL};
        spawnProcess("./visitor", args);
    }
}

void cleanupResources() {
    destroyMsgQueue(visitorCashierMsgQueueId);
    destroyMsgQueue(visitorGuideMsgQueueId1);
    destroyMsgQueue(visitorGuideMsgQueueId2);
    deattachSharedMemory(state);
    destroySharedMemory(shmid);
    destroySemaphore(semId);
}

void handleSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {;
        cleanupResources();
        exit(EXIT_FAILURE);
    }
}