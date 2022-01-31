CC=gcc
CFLAGS=-Wall -Werror

writer: writer.o
	$(CROSS_COMPILE)$(CC) -o writer writer.o $(CFLAGS)

writer.o: writer.c
	$(CROSS_COMPILE)$(CC) -c writer.c $(CFLAGS)

clean:
	@rm -f *.o
	@echo Clean done
