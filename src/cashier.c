#include <stdio.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include "logger.h"
#include "config.h"
#include "utils.h"

volatile sig_atomic_t stop = 0;
void handleSignal(int sig) {
    if (sig == SIGTERM) stop = 1;
}

double calculateTicketPrice(int age, int isRepeat);

void processTicket(sharedState* state, const TicketMessage* msg);


int main(int argc, char* argv[]) {

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGTERM, &signalHandler, NULL);

    int shmid = openSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int visitorCashierMsgQueueId = openMsgQueue(VISITOR_CASHIER_QUEUE_KEY_ID);

    int semId = openSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initLogger(semId);

    LOG("I'm the cashier!");

    while (!stop) {
        TicketMessage msg;
        if (msgrcv(visitorCashierMsgQueueId, &msg, sizeof(TicketMessage) - sizeof(long),
            0, 0) == -1) {
            if (errno == EINTR) continue;
            LOG_ERR("msgrcv");
            continue;
        }
        processTicket(state, &msg);
    }

    deattachSharedMemory(state);
    LOG("Finishing...");
}

double calculateTicketPrice(int age, int isRepeat) {
    if (age < 3) return 0;
    if (isRepeat) return BASE_TICKET_PRICE * 0.5;
    return BASE_TICKET_PRICE;
}

void processTicket(sharedState* state, const TicketMessage* msg) {
    double price = calculateTicketPrice(msg->age, msg->isRepeat);
    state->ticketsSold++;
    state->moneyEarned += price;
    LOG("Ticket sold for %.2f", price);
}