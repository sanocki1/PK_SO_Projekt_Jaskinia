#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logger.h"


int main(int argc, char* argv[]) {
    PRINT("I'm the guard!");

    if (argc != 4) {
        printf("Wrong number of arguments in the guard process\n");
        return 1;
    }

    int Tp = atoi(argv[1]);
    int Tk = atoi(argv[2]);
    int secondsPerHour = atoi(argv[3]);

    if (Tp < 0 || Tk > 24 || Tp >= Tk || secondsPerHour < 1) {
        printf("Invalid time parameters\n");
        return 1;
    }

    int secondsOpen = (Tk - Tp) * secondsPerHour;

    PRINT("Tour is open!");
    PRINT("Tour will close in %d seconds", secondsOpen);

    sleep(secondsOpen);

    PRINT("Tour is closed!");

    return 0;
}
