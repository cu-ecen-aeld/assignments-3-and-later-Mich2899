CROSS_COMPILE=gcc

default:	writer

writer.o:	writer.c
	$(CROSS_COMPILE) -c writer.c

writer:	writer.o
	$(CROSS_COMPILE) writer.o -o writer

clean:
	-rm -f *.o writer



