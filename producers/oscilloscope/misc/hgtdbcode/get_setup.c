#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define HOST "192.168.5.6"
#define PORT 5025

extern int errno;
static struct sockaddr_in server;
static int fd;
static void init_connect();

static char buf[16384];

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


int main(int argc, char** argv)
{
  int size_nb;
  int size_set;
  int nbytes;

  /* === Open connection to Agilent scope */
  init_connect();

  swrite("*IDN?\n");
  if((nbytes = recv(fd, buf, sizeof(buf), 0)) > 0)
    write(STDOUT_FILENO, buf, nbytes);

  swrite(":SYST:SET?\n");
  if (recv(fd, buf, 2, 0) != 2) goto the_end;
  size_nb = buf[1] - '0';
  if (recv(fd, buf, size_nb, 0) != size_nb) goto the_end;
  buf[size_nb] = '\0';
  size_set = atol(buf);

 loop:
  nbytes = recv(fd, buf, sizeof(buf), 0);
  if (nbytes > 0)
    write(STDOUT_FILENO, buf, nbytes);
  else
    goto the_end;
  size_set -= nbytes;
  if (size_set > 0) goto loop;
 
the_end:
  close(fd);
  return 0;
}


static void init_connect()
{
  struct hostent *he;
  if ((he = gethostbyname(HOST)) == NULL)
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
