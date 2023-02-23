CC=gcc

all: default

default: 
	${CROSS_COMPILE}${CC} -o aesdsocket aesdsocket.c -Wall -Werror

clean: 
	$(RM) aesdsocket
	$(RM) /var/tmp/aesdsocketdata
	$(RM) *.o
