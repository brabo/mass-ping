#ifndef PING_H
#define PING_H

#define ICMP_ECHO       8
#define INTV_DEFAULT    1
#define CNTFLAG         (1 << 0)

#define SUCCESS		    0
#define DEFAULT_FAIL	-1
#define OPTS_FAIL	    -2
#define TIMEOUT_FAIL	-3
#define EV_INIT_FAIL	-4

#include "libevquick.h"

struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t chksum;
    uint16_t id;
    uint16_t seq;
};

struct host {
    struct sockaddr_in dest;
    long seq;
    int cnt;
    evquick_timer *tim;
};

struct ping {
    int sock;
    uint8_t flags;
    int max;
    int intv;
    struct host *hosts;
    evquick_event *ev;
};

void usage(char *name);
void ping_quit(int sig);
int parse_ping(uint8_t *in, int inlen, struct sockaddr_in dest);
int parse_opts(int argc, char **argv);

#endif
