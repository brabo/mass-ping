/*
 * Ping - Init Functions
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
#include <unistd.h>
#include <string.h>h
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "events.h"
#include "common.h"
#include "net.h"
#include "ping.h"
#include "libevquick.h"

extern struct ping *ping;

static void setup_exit(void)
{
	//do the sigshit here to add server_exit to some signals (but not 9...)
	signal(SIGINT, ping_quit);
	signal(SIGTERM, ping_quit);
}

static int setup_ping_timer(int cnt)
{
	int ret = DEFAULT_FAIL;

	int time = 1;

	r_log('D', "Ping: setup_ping_timer");
	for (int i = 0; i < cnt; i++) {
		time = i * 50;

		int *perm = malloc(sizeof(int));
		*perm = i;

		ping->hosts[i].tim = evquick_addtimer(time, 0, goping_hdlr, perm);
	}

	ret = SUCCESS;

	return ret;
}

static int setup_timers(int cnt)
{
	int ret = DEFAULT_FAIL;

	setup_ping_timer(cnt);

	ret = SUCCESS;

	return ret;
}

static int setup_events(int cnt)
{
	int ret = DEFAULT_FAIL;

	if(evquick_init() < 0) {
		ret = EV_INIT_FAIL;
	} else {
		ping->ev = evquick_addevent(ping->sock, EVQUICK_EV_READ, read_hdlr, error_hdlr, NULL);
		ret = setup_timers(cnt);
	}

	return ret;
}

int init_shit(int argc, char **argv)
{
	int ret = DEFAULT_FAIL;
	int cnt = 0;

	setup_exit();

	ping = malloc(sizeof(struct ping));

	if((cnt = parse_opts(argc, argv)) == 0 ) {
		exit(OPTS_FAIL);
	}

	ping->sock = socket_setup();
	ret = setup_events(cnt);

	return ret;
}
