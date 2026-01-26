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

int main(int argc, char* argv[]) {
    double price;

    PRINT("I'm the cashier!");

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    sigaction(SIGTERM, &signalHandler, NULL);

    key_t key = generateKey(SHM_KEY_ID);
    int shmid = getShmid(key, 0);
    sharedState* state = attachSharedMemory(shmid);

    key = generateKey(VISITOR_CASHIER_QUEUE_KEY_ID);
    int visitorCashierMsgQueueId = getMsgQueueId(key, 0);

    while (!stop) {
        TicketMessage msg;
        if (msgrcv(visitorCashierMsgQueueId, &msg, sizeof(TicketMessage) - sizeof(long),
            0, 0) == -1) {
            if (errno == EINTR) continue;
            PRINT_ERR("msgrcv");
            continue;
        }
        // PRINT("Visitor age %d", msg.age);
        if (msg.age < 3) { price = 0; }
        else if (msg.isRepeat) { price = BASE_TICKET_PRICE * 0.5; }
        else { price = BASE_TICKET_PRICE; }

        // PRINT("Ticket price: %.2f", price);
        state->ticketsSold++;
        state->moneyEarned += price;
    }

    deattachSharedMemory(state);
    PRINT("Finishing...");
}
