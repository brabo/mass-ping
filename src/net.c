/*
 * Ping - Common Network Functions
 *
 * Author: brabo
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <brabo@cryptolab.net> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.               brabo
 * ----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include "common.h"
#include "ping.h"

extern struct ping *ping;

int socket_setup(void)
{
    int s;
    struct protoent *proto;
 
    if (!(proto = getprotobyname("icmp"))) {
        fprintf(stderr, "ping: unknown protocol icmp.\n");
        exit(1);
    }
 
    if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
        perror("ping: socket");
        exit(2);
    }

    return s;
}

static unsigned short checksum(void *b, int len)
{   unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;
 
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int mk_ping(int n, uint8_t *buf)
{
    int len = 0;

    struct icmp_hdr *icmp = NULL;

    icmp = (struct icmp_hdr *)buf;

    icmp->type = ICMP_ECHO;
    icmp->code = htons(0);
    icmp->chksum = htons(0);
    icmp->id   = (unsigned short) htons(getpid());
    icmp->seq  = ping->hosts[n].seq++;

    len += 8;

    gettimeofday((struct timeval *)&buf[8],
            (struct timezone *)NULL);

    len += 8;
    icmp->chksum = checksum((u_short *)icmp, len);

    return len;
} 
