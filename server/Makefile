CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc

all:	aesdsocket

default:	aesdsocket

writer.o:       aesdsocket.c
	$(CC) -c aesdsocket.c

writer: aesdsocket.o
	$(CC) aesdsocket.o -o aesdsocket

clean:
	-rm -f *.o aesdsocket


