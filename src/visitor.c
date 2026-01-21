#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "logger.h"

int main(int argc, char* argv[]) {
    srand(time(nullptr) + getpid());
    int age = rand() % 80 + 1; // Random age between 1 and 80
    int routeVisited = 0; // 0 - none, 1 - route 1, 2 - route 2

    PRINT("I'm a visitor of age %d", age);

    return 0;
}