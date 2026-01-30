/**
* @file cashier.c
* @brief Proces kasjera.
*
*Proces:
* - odbiera komunikaty od odwiedzających
* - oblicza cenę biletu
* - aktualizuje wspólny stan symulacji
*/
#include <stdio.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "logger.h"
#include "config.h"
#include "utils.h"

volatile sig_atomic_t stop = 0;
void handleSignal(int sig) {
    if (sig == SIGTERM) stop = 1;
}

/** @brief Oblicza cenę biletu dla odwiedzającego. */
double calculateTicketPrice(int age, int isRepeat);

/** @brief Przetwarza zakup biletu i aktualizuje stan symulacji. */
void processTicket(sharedState* state, const TicketMessage* msg);


int main(int argc, char* argv[]) {

    struct sigaction signalHandler = {0};
    signalHandler.sa_handler = handleSignal;
    if (sigaction(SIGTERM, &signalHandler, NULL) == -1) {
        perror("sigaction SIGTERM");
        return EXIT_FAILURE;
    }

    int shmid = openSharedMemory(SHM_KEY_ID);
    sharedState* state = attachSharedMemory(shmid);

    int visitorCashierMsgQueueId = openMsgQueue(VISITOR_CASHIER_QUEUE_KEY_ID);

    int semId = openSemaphore(SEMAPHORE_KEY_ID, SEM_COUNT);
    initLogger(semId);

    LOG("I'm the cashier!");

    while (!stop || state->visitorCount) {
        TicketMessage msg;
        // checks for messages, if received processes it, otherwise tries again to not block at the program end
        if (msgrcv(visitorCashierMsgQueueId, &msg, TICKET_MESSAGE_SIZE,
            0, IPC_NOWAIT) == -1) {
            if (errno == EINTR) continue;
            if (errno == ENOMSG) {
                usleep(100000);
                continue;
            }
            if (errno == ENOMSG && stop) break;
            perror("msgrcv");
            LOG("TUTAJ cashier");
            continue;
        }
        V(semId, TICKET_QUEUE_SEM);
        processTicket(state, &msg);
    }
    LOG("Tickets sold for the day: %d", state->ticketsSold);
    LOG("Total money earned: %.2f", state->moneyEarned);

    deattachSharedMemory(state);

    LOG("Finishing...");
    return 0;
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
    LOG("Ticket sold for %.2f.", price);
}