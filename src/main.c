#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "logger.h"

int main(int argc, char* argv[]) {

    PRINT("I'm the main process!");

    int guardPid = fork();
    if (guardPid == -1) {
        perror("fork");
    }
    if (guardPid == 0) {
        execl("./guard", "guard", argv[1], argv[2], argv[3], NULL);
        perror("execl failed");
    }

    wait(nullptr);

    return 0;
}