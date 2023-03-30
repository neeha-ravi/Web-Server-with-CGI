#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h> 
#include <fcntl.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#define QUEUE_SIZE 50

#ifndef NETH
#define NETH

int create_service(short port);
#endif

