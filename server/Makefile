CROSS_COMPILE=
ifeq ($(CC),)
	CC=$(CROSS_COMPILE)gcc
endif
ifeq ($(CFLAGS),)
	CFLAGS= -Wall -Werror
endif
ifeq ($(LDFLAGS),)
	LDFLAGS= -pthread -lrt
endif

all:	aesdsocket

default:	aesdsocket

aesdsocket.o:       aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c $(LDFLAGS)

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS)  aesdsocket.o -o aesdsocket $(LDFLAGS)

clean:
	-rm -f *.o aesdsocket


