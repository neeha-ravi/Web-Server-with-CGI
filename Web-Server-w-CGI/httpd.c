#define _GNU_SOURCE
#include "httpd.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int create_service(short port) {
   int fd;  /* listen on sock_fd, new connection on new_fd */
   struct sockaddr_in local_addr;    /* my address information */
   int yes=1;

   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      return -1;
   }

   if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,&yes, sizeof(int)) == -1)
   {
      return -1;
   }
        
   local_addr.sin_family = AF_INET;         /* host byte order */
   local_addr.sin_port = htons(port);       /* short, network byte order */
   local_addr.sin_addr.s_addr = INADDR_ANY; /* automatically fill with my IP */
   memset(&(local_addr.sin_zero), '\0', 8); /* zero the rest of the struct */

   if (bind(fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1)
   {
      return -1;
   }

   if (listen(fd, QUEUE_SIZE) == -1)
   {
      return -1;
   }

   return fd;
}

int accept_connection(int fd) {
   int new_fd;
   struct sockaddr_in remote_addr;
   socklen_t size = sizeof(struct sockaddr_in);

   errno = EINTR;
   while (errno == EINTR)
   {
      if ((new_fd = accept(fd, (struct sockaddr*)&remote_addr, &size)) == -1
         && errno != EINTR) {
         return -1;
         }
      else if (new_fd != -1) {
         break;
      }
   }
   return new_fd;
}

int gorh(char *x, int nfd) {
   if (strcmp(x, "GET") == 0) {
      return 7;
   } else if (strcmp(x, "HEAD") == 0) {
      return 8;
   } else {
      char *perr = "HTTP/1.0 500 Internal Error \n";
      write(nfd, perr, strlen(perr));
      close(nfd);
      return 1;
   }
}

void *checked_malloc (size_t size) {
    int *p;
    int len = 100;
    p = malloc(sizeof(uint32_t) * len);
    if (p == NULL) {
        perror("malloc");
        exit(1);
    }
    return p;
}

char *uint32_to_str(uint32_t i) {
   int length = snprintf(NULL, 0, "%lu", (unsigned long)i);       // pretend to print to a string to get length
   char *str = checked_malloc(length + 1);                        // allocate space for the actual string
   snprintf(str, length + 1, "%lu", (unsigned long)i);            // print to string

   return str;
}

void get_items(char *str, char *args[]) {
   char delim[2] = " ";
   char *token;
   int x = 0;
   while ((token = strsep(&str, delim)) != NULL) {
      args[x] = token;
      x ++;
   }
}

void get_size(char *f, int nfd) {
   struct stat buffer;
   int check;
   check = lstat(f, &buffer);
   if (check == 0) {
      off_t size = buffer.st_size;
      char *ss = uint32_to_str(size);
      write(nfd, ss, strlen(ss));
      free(ss);
   }
}

int exists(const char *fname) {
   FILE *file;
   if ((file = fopen(fname, "r"))) {
      fclose(file);
      return 1;
   }
    return 0;
}

void split_slash(char *str, char *args[]) {
   char delim[2] = "/";
   char *token;
   int x = 0;
   while ((token = strsep(&str, delim)) != NULL) {
      args[x] = token;
      x ++;
   }
}

int split_q(char *str, char *args[]) {
   char delim[2] = "?";
   char *token;
   int x = 0;
   while ((token = strsep(&str, delim)) != NULL) {
      args[x] = token;
      x ++;
   }
   return x;
}

void line_two_three(char *file, int nfd) {
   char *line2 = "Content-Type: text/html\r\n";
   write(nfd, line2, strlen(line2));
   char *line3 = "Content-Length: ";
   write(nfd, line3, strlen(line3));
   get_size(file, nfd);
}

int line_one(char *file, int nfd ) {
   char *line1 = "HTTP/1.0 200 OK\r\n";   
   if (file[0] == '/') {
      memmove(file, file+1, strlen(file));
   }
   if (exists(file) == 1) {
      write(nfd, line1, strlen(line1));
      line_two_three(file, nfd);
      char *end = "\r\n";
      write(nfd, end, strlen(end));
   } else {
      char *nferror = "HTTP/1.0 404 Not Found \n";
      write(nfd, nferror, strlen(nferror));
      printf("file does not exist\n");
      close(nfd);
      return 1;
   }
   return 0;
}

void child(char *path) {
   remove("test");
   int fd = open("test", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
   char *sarg[2000];
   int x = 0;
   x = split_q(path, sarg);
   dup2(fd, 1);
   close(fd);
   char *file = sarg[0];
   sarg[x] = NULL;
	execv(file, sarg);
   perror("exec");
   exit(1);
}

void GET_reply(int goh, int nfd, char *file) {
   char *e = "\r\n";
   write(nfd, e, strlen(e));
   if (goh == 7) {
      FILE *f = fopen(file, "r");
      if ( f == NULL) {
         char *err = "HTTP/1.0 501 Not Implemented \n";
         write(nfd, err, strlen(err));
         close(nfd);
         exit(1);
      }
      char *l = NULL;
      size_t size;
      while (getline(&l, &size, f) > 0) {
         write(nfd, l, strlen(l));
      }
      free(l);
      fclose(f);
   }
}

void cgi(int nfd, char *path) {
   pid_t c;
   char *sarg[2000];
   char *test = strdup(path);
   int x;
   x = split_q(test, sarg);
   if (exists(sarg[0]) != 1) {
      char *nferror = "HTTP/1.0 404 Not Found \n";
      write(nfd, nferror, strlen(nferror));
      printf("file does not exist\n");
      close(nfd);
      exit(1);
   }

   if ((c = fork()) == 0) {
      char *line1 = "HTTP/1.0 200 OK\r\n";
      write(nfd, line1, strlen(line1));
      line_two_three("test", nfd);
      child(path);
      char *end = "\r\n";
      write(nfd, end, strlen(end));
      exit(0);
   } else if (c == -1) {
      char *err = "HTTP/1.0 500 Internal Error \n";
      write(nfd, err, strlen(err));
      close(nfd);
      exit(1);
   } else {
      int status;
      wait(&status);
   }
   free(test);
   return ;
}

void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r");
   char *line = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL)
   {
      perror("fdopen");
      char *err = "HTTP/1.0 400 Bad Request";
      write(nfd, err, strlen(err)); 
      close(nfd);
      return;
   }
   int count = 0;
   int goh;
   int cot;
   while ((num = getline(&line, &size, network)) >= 0) {
      // line = received data
      char *args[2000];
      get_items(line, args);
      if (count == 0) {
         goh = gorh(args[0], nfd);
         char *path = args[1];
         if (path[0] == '/') {
            memmove(path, path+1, strlen(path));
         }
         char *sarg[2000];
         char *test = strdup(args[1]);
         split_slash(test, sarg);
         if (strcmp(sarg[0], "cgi-like") == 0) {
            cot = 5;
            cgi(nfd, path);
            char *e = "\r\n";
            write(nfd, e, strlen(e));
         } else if (strcmp(sarg[0], "..") == 0) {
            char *pde = "HTTP/1.0 403 Permission Denied \r\n";
            write(nfd, pde, strlen(pde));
            char *z = "\r\n";
            write(nfd, z, strlen(z));
            close(nfd);
         } else {
            cot = 4;
            line_one(args[1], nfd);
         }

         if (cot == 4) {
            GET_reply(goh, nfd, args[1]);
         } else if (cot == 5) {
            GET_reply(goh, nfd, "test");
         }

         // char *dsaf = "TEST THE END \n";
         // write(nfd, dsaf, strlen(dsaf));
         free(test);
      }
      count ++;
   }
   free(line);
   fclose(network);
}

void run_service(int fd) {
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd != -1)
      {
         if (fork() == 0) {
            // child
            printf("Connection established\n");
            handle_request(nfd);
            printf("Connection closed\n");
            exit(0);
         }
         else {
            // parent
            int status;
            wait(&status);
         }
      }
   }
}

void set_port(int port) {
   if (port == 80) {
      printf("FAILED: webservers generally use port 80\n");
      exit(1);
   }
   else if (port >= 1024 && port <= 65535) {
      printf("listening on port: %d\n", port);
   }
   else {
      printf("FAILED: port out of range \n");
      exit(1);
   }
}

int main(int argc, char *argv[])
{
   if (argc == 1) {
      printf("FAILED: no specified port \n");
      exit(1);
   }
   int port = atoi(argv[1]);
   set_port(port);
   int fd = create_service(port);

   if (fd != -1) {
      run_service(fd);
      close(fd);
   }
   
   return 0;
}
