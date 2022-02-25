INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS= 
CFLAGS= -Wall -Werror -g -I. -pthread
LIBS= 

HFILES= 
CFILES= aesdsocket.c queue.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:    aesdsocket queue.o

clean:
		-rm -f *.o *.d
		-rm -f aesdsocket 

aesdsocket: aesdsocket.o queue.o
		$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $@.o queue.o


aesdsocket.o: aesdsocket.c
		$(CROSS_COMPILE)$(CC)  -c $^
	

queue.o:queue.c
		$(CROSS_COMPILE)$(CC) -c $^ 
