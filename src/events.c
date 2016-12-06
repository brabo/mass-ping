/*
 * Ping - Event Handlers
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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "events.h"
#include "common.h"
#include "net.h"
#include "ping.h"
#include "libevquick.h"

extern struct ping *ping;

void error_hdlr(int fd, short rev, void *arg)
{
	r_log('D', "%d\t :  ERROR CB", fd);
}

void goping_hdlr(void *arg)
{
    int *n = (int *)arg;
    ping->hosts[*n].cnt++;
    if (ping->hosts[*n].cnt < ping->max) {
        int time = 1000 * ping->intv;
        ping->hosts[*n].tim = evquick_addtimer(time, 0, goping_hdlr, arg);
    }
    r_log('D', "pinging #%i", *n);
    int outlen, i;
    uint8_t out[1024];
    char *hostname = NULL;
    struct sockaddr_in dest = {};

    outlen = mk_ping(*n, out);

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ping->hosts[*n].dest.sin_addr.s_addr;

    r_log('D', "Got ip 0x%08X", dest.sin_addr);
 
    i = sendto(ping->sock, (char *)&out, outlen, 0, (struct sockaddr *)&dest,
        sizeof(dest));

    return 0;
}

void read_hdlr(int fd, short rev, void *arg)
{
    int i, inlen, maxin = 1024;
    uint8_t in[1024];
    struct sockaddr_in dest = {};

    int fromlen = sizeof (dest);
 
    if ((inlen = recvfrom(ping->sock, (char *)in, maxin, 0,
        (struct sockaddr *)&dest, (socklen_t *)&fromlen)) < 0) {
        if (errno == EINTR) {
            perror("ping: recvfrom");
            return;
        }
        perror("ping: recvfrom");
        return;
    } else {
        parse_ping(in, inlen, dest);
    }
}


