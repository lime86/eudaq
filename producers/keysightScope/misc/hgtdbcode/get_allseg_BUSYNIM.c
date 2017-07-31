#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RAW_DATA_ENABLED
#define WITH_TTAG

#define HOST "128.141.203.189"
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


/* ----------------------------------------------------------------------0
   Transfer binary data file from scope to disk
   in Keysight STREAMING format

   id = fileid of binary data file
   nbytes = expected record length

   Uses global buffer space [buf]
   Returns 0 if success

   Will remove first two bytes '#0' and last byte '\n'
   ---------------------------------------------------------------------- */
int scope_data2disk(int id_dat, int nbytes){
  int nrec;
  int nhdr = 2;
  int ntrl = 0;
  while (nbytes > 0){
    nrec = recv(fd, buf, sizeof(buf), 0);
    if (nrec <= 0)
      break;
    nbytes -= nrec;
    ntrl = (nbytes == 0);
    if (id_dat > 0) write(id_dat, buf+nhdr, nrec-nhdr-ntrl);
    nhdr = 0;
  }
  if (nbytes != 0 || buf[nrec-1] != '\n')
    return 1;
  else
    return 0;
}


int main(int argc, char** argv)
{
  struct timeval tv;

  double sps_cycle = 0.;
  int sps_cycle_sec;
  int sps_cycle_usec;

  time_t      nxt_sec;
  suseconds_t nxt_usec;
  time_t      tnow;

  int id_dat = 0;
  int id_txt = 0;

  int ier;
  int nsegm;
  int nbytes, nel;
  int i;
  int nchannels = 4;
  int chmask    = 0;

  int pre_fmt;
  int pre_type;
  int pre_npts;

  sigset_t signalMask;

  int flag_the_end = 0;

  /* Fine time stamp is also used to synchronize with SPS cycle */
  gettimeofday(&tv, NULL);
  nxt_sec  = tv.tv_sec;
  nxt_usec = tv.tv_usec;

  /* The command-line argument may be the supercycle length in seconds */
  if (argc > 1){
    int    scl;
    double eps;
    sps_cycle = atof(argv[1]);
    scl = (int)(sps_cycle / 1.2 + 0.5);
    eps = sps_cycle - 1.2 * scl;
    if (sps_cycle < 10.0 || eps < -1.0E-3 || eps > 1.0E-3){
      fprintf(stderr, "%s does not seem a correct supercycle length\n",
	      argv[1]);
      return 1;
    }
  }

  sps_cycle_sec  = (int)sps_cycle;
  sps_cycle_usec = (int)(10.*sps_cycle + 0.5) - 10 * sps_cycle_sec;
  sps_cycle_usec = sps_cycle_usec * 100000;

#ifdef RAW_DATA_ENABLED
  /* Open Binary output data file */
  snprintf(buf, sizeof(buf), "data_%ld.dat", nxt_sec);
  id_dat = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (id_dat < 0){
    perror(buf);
    return 1;
  }
#endif

  /* Open ASCII output data (meta) file */
  snprintf(buf, sizeof(buf), "data_%ld.txt", nxt_sec);
  id_txt = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (id_txt < 0){
    perror(buf);
    return 1;
  }

  /* === Open connection to Agilent scope */
  init_connect();

  nel = swrite("*IDN?\n");
  if (nel <= 0)
    goto the_end;
  if((nbytes = recv(fd, buf, sizeof(buf), 0)) > 0)
    write(STDERR_FILENO, buf, nbytes);

  /* Set busy signal? Not much point right now, do it anyway */
  nel = swrite(":CAL:OUTP DC,-0.8\n");
  if (nel <= 0)
    goto the_end;

  nel = swrite(":SYST:HEAD 0\n");
  if (nel <= 0)
    goto the_end;

  nel = swrite(":WAV:FORM BIN;BYT LSBF;STR 1;SEGM:ALL 1\n");
  if (nel <= 0)
    goto the_end;

#ifdef CHECK_SYNTAX
  swrite(":SYST:ERR? STR\n");
  if((nbytes = recv(fd, buf, sizeof(buf), 0)) > 0)
    write(STDERR_FILENO, buf, nbytes);
#endif


  /* === See what channels are enabled */
  for(i=0; i<nchannels; i++){
    snprintf(buf, sizeof(buf), ":STAT? CHAN%d\n", i+1);
    nel = swrite(buf);
    nbytes = recv(fd, buf, sizeof(buf), 0);
    if (nbytes <= 0)
      goto the_end;
    if (atol(buf)){
      chmask |= (1 << i);
      fprintf(stderr, "Channel %d will be saved\n", i+1);
    }
  }


  /* === We don't want CTRL-C while data is being read out */
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGINT);
  if (sigprocmask(SIG_BLOCK, &signalMask, NULL) < 0)
    perror("sigprocmask()");

 next_cycle:
  swrite(":RUN\n");

  /* Release busy */
  swrite(":CAL:OUTP DC,0\n"); 

  nxt_sec  += sps_cycle_sec;
  nxt_usec += sps_cycle_usec;
  if (nxt_usec > 999999){
    nxt_usec -= 1000000;
    nxt_sec  += 1;
  }

  /* ----------------------------------------------------------------------
    Wait until:
    - acquisition STOPPED
    OR
    - STOP to read fewer segments at the end of the SPS extraction
    OR
    - STOP to read fewer segments if we get CTRL-C while in the waiting loop
    ---------------------------------------------------------------------- */
 wait_loop:
  if (sigpending(&signalMask) < 0){
    perror("sigpending()");
    goto the_end;
  }
  if (sigismember(&signalMask, SIGINT) == 1){
    fputs("\nCTRL-C received\n", stderr);
    flag_the_end = 1;
    /* Set BUSY, allow to propagate */
    swrite(":CAL:OUTP DC,-0.8\n");
    usleep(100000);
    swrite(":STOP\n");
    goto wait_done;
  }
  if (sps_cycle_sec > 0){
    tnow = time(NULL);
    if (tnow > nxt_sec){
      /* Set BUSY, allow to propagate */
      swrite(":CAL:OUTP DC,-0.8\n");
      usleep(100000);
      swrite(":STOP\n");
      goto wait_done;
    }
  }
  swrite(":AST?\n");
  nbytes = recv(fd, buf, sizeof(buf), 0);
  if (nbytes <= 0)
    goto the_end;
  /* Wait for ADONE other states are ARM | TRIG | ATRIG */
  if (buf[1] != 'D'){
    fputc('.', stderr);
    sleep(1);
    goto wait_loop;
  }
  /* Set BUSY (too late) */
  swrite(":CAL:OUTP DC,-0.8\n");

 wait_done:
  /* --- Need to know how many events we have */
  nel = swrite(":WAV:SEGM:COUN?\n");
  if (nel <= 0)
    goto the_end;
  nbytes = recv(fd, buf, sizeof(buf), 0);
  if (nbytes <= 0)
    goto the_end;
  nsegm = atol(buf);
  fprintf(stderr, "\nnevt=%d\n", nsegm);

  if (nsegm == 0)
    goto next_cycle;


  /* --- Read waveform data for all selected channels */
  for(i=0; i<nchannels; i++){
    if ((chmask & (1<<i)) == 0) continue;

    /* Waveform PREamble first and put in the TEXT file */
    snprintf(buf, sizeof(buf), ":WAV:SOUR CHAN%d;PRE?\n", i+1);
    nel = swrite(buf);
    if (nel <= 0)
      goto the_end;
    nbytes = recv(fd, buf, sizeof(buf), 0);
    if (nbytes <= 0)
      goto the_end;
    write(id_txt, buf, nbytes);
    sscanf(buf, "%d,%d,%d", &pre_fmt, &pre_type, &pre_npts);
    fprintf(stderr, "fmt=%d type=%d npts=%d\n",
	    pre_fmt, pre_type, pre_npts);

    /* Waveform data goes to binary file */
    nel = swrite(":WAV:DATA?\n");
    if (nel <= 0)
      goto the_end;
    /* Waveform data will be in 16-bit INTEGER format */
    nbytes = nsegm*pre_npts*sizeof(short)+3;
    ier = scope_data2disk(id_dat, nbytes);
    if (ier){
      fprintf(stderr, "Data check error at %d\n", __LINE__);
      goto the_end;
    }
  }

#ifdef WITH_TTAG
  /* --- Read all Time Tags in BINary FLOAT64 and SAVE TO DISK */
  nel = swrite(":WAV:SEGM:XLIS? TTAG\n");
  if (nel <= 0)
    goto the_end;
  nbytes = nsegm*sizeof(double)+3;
  ier = scope_data2disk(id_dat, nbytes);
  if (ier){
    fprintf(stderr, "Data check error at %d\n", __LINE__);
    goto the_end;
  }
#endif

  if (flag_the_end) goto the_end;

  /* --- Start new sequence */
  swrite(":RUN\n");

  /* Release busy */
  swrite(":CAL:OUTP DC,0\n"); 

  /* --- Schedule next End-of-Fill STOP no earlier than 5 seconds from now */
  if (sps_cycle_sec > 0){
    tnow = time(NULL);
    while (nxt_sec < tnow + 5){
      nxt_sec  += sps_cycle_sec;
      nxt_usec += sps_cycle_usec;
      if (nxt_usec > 999999){
	nxt_usec -= 1000000;
	nxt_sec  += 1;
      }
    }
  }
  goto wait_loop;


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
