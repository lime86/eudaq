#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT 5025

extern int errno;
static struct sockaddr_in server;
static int fd;
static void init_connect();

static char buf[4096];

int swrite(char* buf){
  int len = strlen(buf);
  int val;
  val = send(fd, buf, len, 0);
  if (val <= 0){
    perror(buf);
    return 0;
  }
  return val;
}

static void init_connect(char* hostName)
{
  struct hostent *he;
  if ((he = gethostbyname(hostName)) == NULL)
    exit(errno);
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    exit(errno);

  /* --- connect to the Modbus server */
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr = *((struct in_addr *)he->h_addr);
  bzero(server.sin_zero, 8);

  /* --- connect will fail if the server is not running */
  if (connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1){
    perror("Cannot connect to server");
    exit(errno);
  }
}

int main(int argc, char** argv)
{
  int nbytes, nel;

  if (argc < 2) return 1;

  init_connect(argv[1]);

  nel = swrite("*IDN?\n");
  if (nel <= 0) return 1;

  if((nbytes = recv(fd, buf, sizeof(buf), 0)) > 0)
    write(STDERR_FILENO, buf, nbytes);

  return 0;
}
