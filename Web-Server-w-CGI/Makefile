CC = gcc
CFLAGS = -Wall -pedantic -std=c99
SERVER = httpd
SERVER_OBJS = httpd.o
PROGS = $(SERVER)

all : $(PROGS)

$(SERVER) : $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_OBJS)

httpd.o : httpd.c httpd.h
	$(CC) $(CFLAGS) -c httpd.c

clean :
	rm *.o $(PROGS) core
	