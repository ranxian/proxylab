TEAM = NOBODY
VERSION = 1
HANDINDIR = /afs/cs/academic/class/15213-f02/L7/handin

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread

OBJS = proxy.o csapp.o cache.o crc32.o tswrapper.o

all: proxy tiny

proxy: $(OBJS)
	$(CC) $(OBJS) -o proxy $(LDFLAGS)

tiny: csapp.o tiny.o tswrapper.o
	$(CC) tiny.o csapp.o tswrapper.o -o tiny $(LDFLAGS)

csapp.o: csapp.c 
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c
	$(CC) $(CFLAGS) -c proxy.c

tiny.o: tiny.c
	$(CC) $(CFLAGS) -c tiny.c

cache.o: cache.c
	$(CC) $(CFLAGS) -c cache.c	

crc32.o: crc32.c
	$(CC) $(CFLAGS) -c crc32.c

tswrapper.o: tswrapper.c
	$(CC) $(CFLAGS) -c tswrapper.c

handin:
	cp proxy.c $(HANDINDIR)/$(TEAM)-$(VERSION)-proxy.c

clean:
	rm -f *~ *.o proxy core tiny

