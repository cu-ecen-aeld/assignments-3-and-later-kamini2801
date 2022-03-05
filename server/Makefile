INCLUDE_DIRS =  
LIB_DIRS = 
CC=gcc

CDEFS= 
CFLAGS= -Wall -Werror
LIBS= 

HFILES= 
CFILES= aesdsocket.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:    aesdsocket

clean:
		-rm -f *.o *.d
		-rm -f aesdsocket

aesdsocket: aesdsocket.o
		$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $@.o 


aesdsocket.o: aesdsocket.c
		$(CROSS_COMPILE)$(CC)  -c aesdsocket.c

