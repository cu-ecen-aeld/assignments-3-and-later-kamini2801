ifeq ($(CC),)
	CC = $(CROSS_COMPILE)gcc
endif
ifeq ($(CFLAGS),)
	CFLAGS = -g -Wall -Werror -I. 
endif
ifeq ($(LDFLAGS),)
	LDFLAGS = -pthread -lrt
endif

all:    aesdsocket queue.o

clean:
		-rm -f *.o *.d
		-rm -f aesdsocket 

aesdsocket: aesdsocket.o queue.o
		$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS)



aesdsocket.o: aesdsocket.c
		$(CROSS_COMPILE)$(CC)  -c $^
	

queue.o:queue.c
		$(CROSS_COMPILE)$(CC) -c $^ 
