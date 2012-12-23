TEAM = NOBODY
VERSION = 1
HANDINDIR = /afs/cs/academic/class/15213-f02/L7/handin

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread

OBJS = proxy.o csapp.o

all: proxy tiny

proxy: $(OBJS)
	$(CC) $(OBJS) -o proxy $(LDFLAGS)

tiny: csapp.o tiny.o
	$(CC) tiny.o csapp.o -o tiny $(LDFLAGS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c
	$(CC) $(CFLAGS) -c proxy.c

tiny.o: tiny.c
	$(CC) $(CFLAGS) -c tiny.c

handin:
	cp proxy.c $(HANDINDIR)/$(TEAM)-$(VERSION)-proxy.c

clean:
	rm -f *~ *.o proxy core tiny

