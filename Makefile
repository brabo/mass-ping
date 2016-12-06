CC=gcc
CFLAGS=-I$(HOME)/rivendel/include -I./include -std=gnu99 -m32
LDFLAGS=-L$(HOME)/rivendel/lib
LDLIBS=-lrivendel -lm

PING_OBJ=src/ping.o
NET_OBJ=src/net.o
EV_OBJ=src/events.o
INIT_OBJ=src/init.o

ping: $(NET_OBJ) $(EV_OBJ) $(INIT_OBJ) $(PING_OBJ)
	$(CC) -o $@ $(NET_OBJ) $(EV_OBJ) $(INIT_OBJ) $(PING_OBJ) $(CFLAGS) $(LDFLAGS) $(LDLIBS)

all: ping

clean:
	rm -f src/*.o ping


