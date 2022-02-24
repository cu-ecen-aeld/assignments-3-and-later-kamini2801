INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS= 
CFLAGS= -Wall -Werror -g -I. -lpthread
LIBS= 

HFILES= 
CFILES= aesdsocket.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:    aesdsocket queue

clean:
		-rm -f *.o *.d
		-rm -f aesdsocket queue

aesdsocket: aesdsocket.o
		$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $@.o 


aesdsocket.o: aesdsocket.c
		$(CROSS_COMPILE)$(CC)  -c aesdsocket.c 
	
queue: queue.o
		$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $@.o 


queue.o:queue.c
		$(CROSS_COMPILE)$(CC)  -c queue.c 

