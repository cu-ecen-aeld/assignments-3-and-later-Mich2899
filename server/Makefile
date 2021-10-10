CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
CFLAGS= -Wall -Werror
LIBS= -lpthread -lrt

all:	aesdsocket

default:	aesdsocket

aesdsocket.o:       aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c queue.h $(LIBS)

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS)  aesdsocket.o -o aesdsocket $(LIBS)

clean:
	-rm -f *.o aesdsocket


