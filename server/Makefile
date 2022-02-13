INCLUDE_DIRS =  
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
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
		$(CC) $(CFLAGS) -o $@ $@.o 


aesdsocket.o: aesdsocket.c
		$(CC)  -c aesdsocket.c

