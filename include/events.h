#ifndef EVENTS_H
#define EVENTS_H

/*
 * Event handlers
 */
void goping_hdlr(void *arg);
void read_hdlr(int fd, short rev, void *arg);
void error_hdlr(int fd, short rev, void *arg);

#endif
