/*
 * Ping - event based multi-ping
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

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "libevquick.h"
#include "common.h"
#include "net.h"
#include "init.h"
#include "events.h"
#include "ping.h"
 
extern FILE *rlog;
extern int verbose;
struct ping *ping;

void usage(char *name)
{
    printf("Usage: %s [-v] [-c count] [-i interval] destination\n",
            name);
}

void ping_quit(int sig)
{
    r_log('I', "Ping :  Shutting down due to signal %d.\n", sig);
    evquick_fini();
    log_close();

    evquick_delevent(ping->ev);
    close(ping->sock);
    for (int i = 0; i < 254; i++) {
        evquick_deltimer(ping->hosts[i].tim);
    }
    free(ping->hosts);
    free(ping);

    // if you're happy and you know it,
    exit(0);
}

static void tvsub(register struct timeval *out, register struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0) {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

int parse_ping(uint8_t *in, int inlen, struct sockaddr_in dest)
{
    struct icmp_hdr *icmp;

    struct timeval tv, *tp;
    u_long triptime = 0;
    uint16_t ident = htons(getpid());

    gettimeofday(&tv, (struct timezone *)NULL);

    int hlen;
    struct iphdr *ip;

    static long tmin = LONG_MAX;    /* minimum round trip time */
    static long tmax = 0;       /* maximum round trip time */
    static u_long tsum;     /* sum of all times, for doing average */
    /* Check the IP header */
    ip = (struct iphdr *)in;
    hlen = ip->ihl << 2;


    /* Now the ICMP part */
    inlen -= hlen;
    icmp = (struct icmp_hdr *)(in + hlen);
    if (icmp->type == ICMP_ECHOREPLY) {
        if (icmp->id != ident)
            return 1;           /* 'Twas not our ECHO */
        tp = (struct timeval *)(in + hlen + sizeof (icmp) + 4);
        tvsub(&tv, tp);
        triptime = tv.tv_sec * 10000 + (tv.tv_usec / 100);
        tsum += triptime;
        if (triptime < tmin)
            tmin = triptime;
        if (triptime > tmax)
            tmax = triptime;
    }
    char host[64];
    sprintf(host, "%s", inet_ntoa(*(struct in_addr *)&dest.sin_addr.s_addr));
    if (strcmp(host, "192.168.0.213")) {
        printf("%d bytes from %s: icmp_seq=%u", inlen,
            inet_ntoa(*(struct in_addr *)&dest.sin_addr.s_addr),
            icmp->seq);
        printf(" ttl=%d", ip->ttl);
        printf(" time=%ld.%ld ms\n", triptime/10,
            triptime%10);
    }
    return 0;

}

struct cidr {
    char *ip;
    char *cidr;
};

int cidr_parse(struct cidr *cidr, char *arg)
{
    int len, iplen, cidrlen;

    len = strlen(arg);
    cidr->ip = strtok(arg, "/");
    iplen = strlen(cidr->ip);

    if (iplen < len) {
        cidr->cidr = strtok(NULL, "/");
        cidrlen = strlen(cidr->cidr);
    } else {
        cidr->cidr = malloc((sizeof (char)) * 8);
        memcpy(cidr->cidr, "32", 3);
        cidrlen = 2;
    }

    return 0;
}

int populate_ping(struct cidr *cidr)
{
    int togo, todo;

    togo = 32 - atoi(cidr->cidr);

    todo = pow(2, togo);

    uint32_t tip;
    inet_aton(cidr->ip, &tip);
    uint32_t ip = ntohl(tip);

    while (togo--) {
        ip &= ~(1 << togo);
    }

    int j = 0;
    
    ping->hosts = malloc(sizeof (struct host) * todo);
    for (int i = 0; i < todo; i++) {
        if (((ip & 0xFF) == 0x0) || ((ip & 0xFF) == 0xFF)) {
            ip++;
            continue;
        }

        ping->hosts[j].dest.sin_addr.s_addr = (in_addr_t)htonl(ip);
        ping->hosts[j].cnt = 0;
        ping->hosts[j].seq = 0;
        j++;
        ip++;
    }

    return j;
}

int parse_opts(int argc, char **argv)
{
    char c;
    int cnt = 0;
    extern int optind, optopt;
    extern char *optarg;


    if (argc < 2) {
        fprintf(stderr, "Usage: ping [-v]... [-c count] [-i interval] destination\n");
        exit(1);
    }

    ping->flags = 0;
    ping->max = 0;
    ping->intv = INTV_DEFAULT;
    verbose = 0;
    while ((c = getopt(argc, argv, "vc:i:")) != -1) {
        switch (c) {
        case 'c':
            ping->flags |= CNTFLAG;
            ping->max = atoi(optarg);
            if (ping->max <= 0) {
                fprintf(stderr, "ping: bad number of packets to transmit.\n");
                exit(2);
            }
            break;
        case 'v':
            verbose = 1;
            break;
        case 'i':
            ping->intv = atoi(optarg);
            if (ping->intv <= 0) {
                fprintf(stderr, "ping: bad interval.\n");
                exit(2);
            }
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (!argc) {
        usage(argv[0]);
    }

    int len = strlen(argv[argc - 1]);
    struct cidr *cidr = malloc(sizeof (struct cidr));
    cidr_parse(cidr, argv[argc - 1]);
    cnt = populate_ping(cidr);

    return cnt;
}

int main(int argc, char **argv)
{
    rlog = stdout;
    setbuf(stdout, NULL);

    init_shit(argc, argv);
    evquick_loop();
}

