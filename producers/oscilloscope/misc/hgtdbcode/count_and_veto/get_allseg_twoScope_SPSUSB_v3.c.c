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

/* ----------------------------------------------------------------------
   Derived from get_allseg_BUSYNIM_SPSUSB.c
   No more pseudo BUSY generation
   S,Simion 02 Nov 2016
   ---------------------------------------------------------------------- */

#define CHECK_SYNTAX
#define RAW_DATA_ENABLED
#define WITH_TTAG
#define WITH_BUSY

#define HOST  "192.168.4.70"
//#define HOST2 "192.168.4.71"
#define PORT 5025

extern int errno;
static struct sockaddr_in sa_scope[2];
static int fd[2];
static void init_connect();

static char buf[16384];

int swrite(char* buf, int scope_num){
  int val;
  int len = strlen(buf);
  if (fd[0] != 0 && scope_num != 1){
    val = send(fd[0], buf, len, 0);
    if (val <= 0){
      perror(buf);
      return 0;
    }
  }
  if (fd[1] != 0 && scope_num != 0){
    val = send(fd[1], buf, len, 0);
    if (val <= 0){
      perror(buf);
      return 0;
    }
  }
  return 1;
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
int scope_data2disk(int id_dat, int nbytes, int scope_num){
  int nrec;
  int nhdr = 2;
  int ntrl = 0;
  while (nbytes > 0){
    nrec = recv(fd[scope_num], buf, sizeof(buf), 0);
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


int scope_data2mem(char* large_buf, int nbytes, int scope_num)
{
  int  nrec;
  short hdr;
  recv(fd[scope_num], &hdr, 2, 0);
  if (hdr != 0x3023) return 1;
  nbytes -= 2;
  while (nbytes > 0){
    nrec = recv(fd[scope_num], large_buf, nbytes, 0);
    if (nrec <= 0)
      break;
    nbytes -= nrec;
    large_buf += nrec;
  }
  if (nbytes != 0 || large_buf[-1] != '\n')
    return 1;
  else
    return 0;
}


int main(int argc, char** argv)
{
  struct timeval tv;

  time_t      nxt_sec;
  suseconds_t nxt_usec;

  int id_dat = 0;
  int id_txt = 0;

  int ier;
  int nsegm;
  int nsegm1;
  int nbytes, nel;
  int i;
  int chmask = 0;

  int pre_fmt;
  int pre_type;
  int pre_npts;

  sigset_t signalMask;

  int flag_the_end = 0;

  int nevt_tot = 0;

  unsigned int cycle_count;
  unsigned int cycle_count_old;
  

  /* Fine time stamp is NO LONGER used to synchronize with SPS cycle */
  gettimeofday(&tv, NULL);
  nxt_sec  = tv.tv_sec;
  nxt_usec = tv.tv_usec;

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

  /* === Open and initialize USB counter device */
  mcp_init();

  /* === Open connection to Agilent scope(s) */
  init_connect();

  /* === Scope(s) setup before starting DAQ */
  nel = swrite("*IDN?\n", 0);
  if (nel <= 0)
    goto the_end;
  if (fd[0] != 0 && (nbytes = recv(fd[0], buf, sizeof(buf), 0)) > 0)
    write(STDERR_FILENO, buf, nbytes);
  if (fd[1] != 0){
    nel = swrite("*IDN?\n", 1);
    if (nel <= 0)
      goto the_end;
    if (fd[1] != 0 && (nbytes = recv(fd[1], buf, sizeof(buf), 0)) > 0)
      write(STDERR_FILENO, buf, nbytes);
  }

#ifdef WITH_BUSY
  /* Set busy signal? Not much point right now, do it anyway */
  nel = swrite(":CAL:OUTP DC,-0.8\n",0);
  if (nel <= 0)
    goto the_end;
#endif  

  nel = swrite(":STOP\n", -1);
  if (nel <= 0)
    goto the_end;

  nel = swrite(":SYST:HEAD 0\n", -1);
  if (nel <= 0)
    goto the_end;

  nel = swrite(":WAV:FORM BIN;BYT LSBF;STR 1;SEGM:ALL 1\n", -1);
  if (nel <= 0)
    goto the_end;

  /* Make sure we turn off sin(x)/x interpolation SSIM 01-NOV-2016 */
  nel = swrite(":ACQ:MODE SEGM;INT 0\n", -1);
  if (nel <= 0)
    goto the_end;

  /* Slave scope specific setup */
  swrite(":TIM:REFC 1\n",  1);
  swrite(":TRIG:SWE TRIG\n",  1);
  swrite(":TRIG:MODE EDGE\n", 1);
  swrite(":TRIG:LEV AUX,0.5\n", 1);
  swrite(":TRIG:EDGE:COUP DC;SLOP POS;SOUR AUX\n", 1);

#ifdef CHECK_SYNTAX
  swrite(":SYST:ERR? STR\n", -1);
  if(fd[0] != 0 && (nbytes = recv(fd[0], buf, sizeof(buf), 0)) > 0)
    write(STDERR_FILENO, buf, nbytes);
  if(fd[1] != 0 && (nbytes = recv(fd[1], buf, sizeof(buf), 0)) > 0)
    write(STDERR_FILENO, buf, nbytes);
#endif

  /* === See what channels are enabled on each scope */
  for(i=0; i<4; i++){
    snprintf(buf, sizeof(buf), ":STAT? CHAN%d\n", i+1);
    nel = swrite(buf, -1);
    if (fd[0] != 0){
      nbytes = recv(fd[0], buf, sizeof(buf), 0);
      if (nbytes <= 0)
	goto the_end;
      if (atol(buf)){
	chmask |= (1 << i);
      }
    }
    if (fd[1] != 0){
      nbytes = recv(fd[1], buf, sizeof(buf), 0);
      if (nbytes <= 0)
	goto the_end;
      if (atol(buf)){
	chmask |= (1 << i+4);
      }
    }
  }

  for(i=0; i<8; i++)
    if (chmask & (1 << i))
      fprintf(stderr, "Channel %d will be saved\n", i+1);

  /* === We don't want CTRL-C while data is being read out */
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGINT);
  if (sigprocmask(SIG_BLOCK, &signalMask, NULL) < 0)
    perror("sigprocmask()");

  /* === Get Slave scope ready for accepting triggers */
  swrite(":RUN\n", 1);
  usleep(100000);

 next_cycle:
  swrite(":RUN\n", 0);
  
#ifdef WITH_BUSY
  /* Release busy */
  swrite(":CAL:OUTP DC,0\n",0); 
#endif    

  /* === Get current pulse count (was never reset, does not matter) */
  ier = mcp_get_pulse_count(&cycle_count_old);
  if (ier != 0){
    fputs("Cannot read USB counter\n", stderr);
    return 1;
  }

  /* ----------------------------------------------------------------------
    Wait until:
    - acquisition STOPPED on MASTER SCOPE #1
    OR
    - STOP to read fewer segments at the end of the SPS extraction
    OR
    - STOP to read fewer segments if we get CTRL-C while in the waiting loop

    SCOPE #2 will be the slave always getting triggers from SCOPE #1

    MAKE SURE, BY OTHER MEANS,
    THAT THE MAX NUMBER OF SEGMENTS IS NOT LIMITED BY SCOPE #2

    WITH THE CORRECT HARDWARE SETUP, WE HAVE AN EQUAL NUMBER OF EVENTS
    ON BOTH SCOPES

    Note 06-NOV-2016
    It could be OK after all to have fewer events on SCOPE #2
    ---------------------------------------------------------------------- */
 wait_loop:
  if (sigpending(&signalMask) < 0){
    perror("sigpending()");
    goto the_end;
  }
  if (sigismember(&signalMask, SIGINT) == 1){
    fputs("\nCTRL-C received\n", stderr);
    flag_the_end = 1;
#ifdef WITH_BUSY
    /* Set BUSY, allow to propagate */
    swrite(":CAL:OUTP DC,-0.8\n",0);
    usleep(100000);
#endif		  
    swrite(":STOP\n", 0);
    goto wait_done;
  }
  ier = mcp_get_pulse_count(&cycle_count);
  if ((cycle_count-cycle_count_old)>=5000){
#ifdef WITH_BUSY
    /* Set BUSY, allow to propagate */
    swrite(":CAL:OUTP DC,-0.8\n",0);
    usleep(100000);
#endif		  
    swrite(":STOP\n", 0);
    goto wait_done;
  }
  swrite(":AST?\n", 0);
  nbytes = recv(fd[0], buf, sizeof(buf), 0);
  if (nbytes <= 0)
    goto the_end;
  /* Wait for ADONE other states are ARM | TRIG | ATRIG */
  if (buf[1] != 'D'){
    fputc('.', stderr);
    sleep(1);
    goto wait_loop;
  }
#ifdef WITH_BUSY
  /* Set BUSY (too late) */
  swrite(":CAL:OUTP DC,-0.8\n",0);
  fputc('Readout too late',stderr);
#endif
  
 wait_done:
  /* --- Need to know how many events we have */
  nel = swrite(":WAV:SEGM:COUN?\n", 0);
  if (nel <= 0)
    goto the_end;
  nbytes = recv(fd[0], buf, sizeof(buf), 0);
  if (nbytes <= 0)
    goto the_end;
  nsegm = atol(buf);
  nevt_tot += nsegm;
  fprintf(stderr, "\nnevt=%6d total=%10d\n", nsegm, nevt_tot);

  /* ----------------------------------------------------------------------
     Put slave scope in STOP mode:
     - whenever there are events to read, OR
     - before we exit with CTRL-C
     Do not put slave scope in STOP mode:
     - if we paused just to query number of events, but found none
     ---------------------------------------------------------------------- */
  if (nsegm != 0 || flag_the_end)
    swrite(":STOP\n", 1);

  if (nsegm == 0){
    if (flag_the_end == 0)
      goto next_cycle;
    else
      goto the_end;
  }

  /* --- Number of events in slave scope is UNKNOWN yet */
  nsegm1 = 0;

  /* --- Read waveform data for all selected channels */
  for(i=0; i<8; i++){
    int scope_num;

    if ((chmask & (1<<i)) == 0) continue;

    scope_num = (i > 3);

    /* Waveform PREamble first and put in the TEXT file */
    snprintf(buf, sizeof(buf), ":WAV:SOUR CHAN%d;PRE?\n", (i & 3) + 1);
    nel = swrite(buf, scope_num);
    if (nel <= 0)
      goto the_end;
    nbytes = recv(fd[scope_num], buf, sizeof(buf), 0);
    if (nbytes <= 0)
      goto the_end;
    write(id_txt, buf, nbytes);
    sscanf(buf, "%d,%d,%d", &pre_fmt, &pre_type, &pre_npts);

    /* Slave scope may have fewer events */
    if (scope_num > 0 && nsegm1 == 0){
      char* s;
      buf[nbytes] = '\0';
      s = strrchr(buf, ',');
      if (s != NULL) nsegm1 = atol(s+1);
    }

    /* Waveform data goes to binary file */
    nel = swrite(":WAV:DATA?\n", scope_num);
    if (nel <= 0)
      goto the_end;
    /* Waveform data will be in 16-bit INTEGER format */
    nbytes = (scope_num ? nsegm1 : nsegm) * pre_npts * sizeof(short) + 3;
    ier = scope_data2disk(id_dat, nbytes, scope_num);
    if (ier){
      fprintf(stderr, "Data check error at %d\n", __LINE__);
      goto the_end;
    }
  }

#ifdef WITH_TTAG
  /* --- Read all Time Tags in BINary FLOAT64 and SAVE TO DISK */
  nel = swrite(":WAV:SEGM:XLIS? TTAG\n", -1);
  if (nel <= 0)
    goto the_end;

  /* --- Get them from the Scope#2 first so that we can restart right away */
  nbytes = nsegm1 * sizeof(double) + 3;
  ier = scope_data2disk(id_dat, nbytes, 1);
  if (ier){
    fprintf(stderr, "Data check error at %d\n", __LINE__);
    goto the_end;
  }

  /* --- Slave scope may be restarted now */
  swrite(":RUN\n", 1);

  /* --- Get them from the main scope */
  nbytes = nsegm  * sizeof(double) + 3;
  ier = scope_data2disk(id_dat, nbytes, 0);
  if (ier){
    fprintf(stderr, "Data check error at %d\n", __LINE__);
    goto the_end;
  }
#else
  /* --- If not reading the tags, little sleep for sequencing */
  swrite(":RUN\n", 1);
  usleep(100000);
#endif

  if (flag_the_end) goto the_end;

  /* --- Start new sequence */
  goto next_cycle;

the_end:
  if (fd[0] != 0) close(fd[0]);
  if (fd[1] != 0) close(fd[1]);
  return 0;
}


static void init_connect()
{
  struct hostent *he;

  if ((fd[0] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("init_connect()");
    exit(1);
  }
  if ((he = gethostbyname(HOST)) == NULL){
    herror(HOST);
    exit(1);
  }
  /* --- connect to the scope */
  sa_scope[0].sin_family = AF_INET;
  sa_scope[0].sin_port = htons(PORT);
  sa_scope[0].sin_addr = *((struct in_addr *)he->h_addr);
  bzero(sa_scope[0].sin_zero, 8);
  if (connect(fd[0], (struct sockaddr *)&sa_scope[0],
	      sizeof(struct sockaddr)) == -1){
    perror(HOST);
    exit(1);
  }

#ifdef HOST2
  if ((fd[1] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("init_connect()");
    exit(1);
  }
  if ((he = gethostbyname(HOST2)) == NULL){
    herror(HOST2);
    exit(1);
  }
  /* --- connect to the scope */
  sa_scope[1].sin_family = AF_INET;
  sa_scope[1].sin_port = htons(PORT);
  sa_scope[1].sin_addr = *((struct in_addr *)he->h_addr);
  bzero(sa_scope[1].sin_zero, 8);
  if (connect(fd[1], (struct sockaddr *)&sa_scope[1],
	      sizeof(struct sockaddr)) == -1){
    perror(HOST2);
    exit(1);
  }
#endif
}
