CROSS_COMPILE=gcc

CFLAGS=


DEPS=
OBJ=writer.o

%.o: %.c $(DEPS)
	$(CROSS_COMPILE) $(CFLAGS) -c -o $@ $<

default:	$(OBJ)
	$(CROSS_COMPILE) $(CFLAGS) -o $@ $^

clean:
	rm -r $(OBJ) default



