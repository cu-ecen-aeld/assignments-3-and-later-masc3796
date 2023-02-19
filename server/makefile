all: default

default: 
	${CROSS_COMPILE}gcc -o aesdsocket aesdsocket.c -Wall -Werror

clean: 
	$(RM) aesdsocket
	$(RM) /var/tmp/aesdsocketdata
