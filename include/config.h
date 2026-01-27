// simulation config constants/rules

#ifndef CONFIG_H
#define CONFIG_H

#define OPENING_TIME 8 // Tp
#define CLOSING_TIME 16 // Tk
#define ROUTE_1_CAPACITY 28 // N1
#define ROUTE_2_CAPACITY 35 // N2
#define BRIDGE_CAPACITY 15 // K < Ni
#define ROUTE_1_DURATION 3 // T1
#define ROUTE_2_DURATION 4 // T2
#define BRIDGE_DURATION 2
#define SECONDS_PER_HOUR 3 // IRL duration is opening hours * seconds per hour
#define VISITOR_FREQUENCY 2
#define MAX_VISITOR_GROUP_SIZE 10
#define BASE_TICKET_PRICE 10

#endif