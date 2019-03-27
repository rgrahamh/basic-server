CC=gcc
CFLAGS= -Wall -Wextra -pedantic -Werror

dev: server.c
	$(CC) $(CFLAGS) -o server server.c

test: server.c
	$(CC) $(CFLAGS) -o server server.c -DTESTING

clean:
	rm server
