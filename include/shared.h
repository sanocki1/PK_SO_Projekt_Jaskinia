// shared memory data
#ifndef SHARED_H
#define SHARED_H

typedef struct {
    int Tp; // opening time
    int Tk; // closing time
    int closing; // 0 - simulation running, 1 - simulation closing
} sharedState;

#endif