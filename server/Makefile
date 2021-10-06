CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
CFLAGS= -Wall -Werror

all:	aesdsocket

default:	aesdsocket

aesdsocket.o:       aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS)  aesdsocket.o -o aesdsocket

clean:
	-rm -f *.o aesdsocket


