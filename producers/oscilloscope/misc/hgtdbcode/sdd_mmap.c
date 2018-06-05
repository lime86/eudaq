#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h> // types

#define MAX_SEQ 1000

static FILE* fd_txt = NULL;

static int nseq;

static int nch[2];
static int nevt_seq[2][MAX_SEQ];
static int nevt_tot[2];
static float yscale[2][4];
static float yoffst[2][4];
static int npts_evt[2];

static int      id_bin;

static size_t   size;
static caddr_t  file_ptr;
static int16_t* adc_data;

static size_t offst_current = (size_t)-1;
static size_t offst_align;

static int evtnum_seq;
static int iseq_current;

int scope_data_init(const char* asciiFile){
  int i;
  char line[1024];
  char  dt1[1024];
  char  dt0[1024];
  char  sn0[1024];
  char  sn1[1024];
  int   nrec[2];

  struct stat buf;
  size_t size_expected;

  if (fd_txt != NULL)
    fclose(fd_txt);

  if (size > 0){
    assert(adc_data != NULL);
    munmap(adc_data, size);
    size = 0;
  }

  nch[0] = nch[1] = nevt_tot[0] = nevt_tot[1] = 0;
  for(i=0; i<MAX_SEQ; i++)
    nevt_seq[0][i] = 0;
  dt0[0] = '\0';
  sn0[0] = '\0';

  fd_txt = fopen(asciiFile, "r");
  if (fd_txt == NULL){
    perror(asciiFile);
    return 1;
  }

  nrec[0] = nrec[1] = 0;
  while(fgets(line, sizeof(line), fd_txt)){
    int   nel, npts, nev, snum;
    float yinc;
    float yorg;
    nel = sscanf(line, "%*d,%*d,%d,%*d,%*f,%*lf,%*f,%f,%f,%*f,%*d,%*f,%*f,%*f,%*f,%*[^,],%[^,],%[^,],%*d,%*d,%*d,%*d,%*f,%*f,%d",
		 &npts, &yinc, &yorg, dt1, sn1, &nev);

    assert(nel == 6 && npts > 0 && nev > 0);

    /* Figure our which scope this is */
    if (sn0[0] == '\0'){
      memcpy(sn0, sn1, 1024);
      snum = 0;
    }
    else if (strncmp(sn0, sn1, 1024) == 0){
      if (snum == 1 && nch[1] == 0) nch[1] = nrec[1];
      snum = 0;
    }
    else{
      snum = 1;
      if (nch[1] == 0){
	yscale[1][nrec[1]] = yinc;
	yoffst[1][nrec[1]] = yorg;
	nevt_seq[1][0]     = nev;
	npts_evt[1]        = npts;
      }
      else
	nevt_seq[1][nrec[1]/nch[1]] = nev;
      nrec[1]++;
      continue;
    }

    /* Figure out number of channels for master scope */
    if (nch[0] == 0){
      if (dt0[0] == '\0'){
	memcpy(dt0, dt1, 1024);
      }
      if (strncmp(dt0, dt1, 1024) == 0){
	yscale[0][nrec[0]] = yinc;
	yoffst[0][nrec[0]] = yorg;
	nevt_seq[0][0]     = nev;
	npts_evt[0]        = npts;
      }
      else
	nch[0] = nrec[0];
    }
    else{
      nevt_seq[0][nrec[0]/nch[0]] = nev;
    }
    nrec[0]++;
  }

  if (nch[0] == 0) nch[0] = nrec[0];
  if (nch[1] == 0) nch[1] = nrec[1];

  nseq = nrec[0]/nch[0];

  for(i=0; i<MAX_SEQ; i++){
    nevt_tot[0] += nevt_seq[0][i];
    nevt_tot[1] += nevt_seq[1][i];
  }

  for(i=0; i<2; i++)
    printf("#OSC%d nch=%d pts=%d evts=%d\n",
	   i, nch[i], npts_evt[i], nevt_tot[i]);

  /* prep to access real wfm data */
  {
    char *s;
    strncpy(line, asciiFile, sizeof(line));
    s = strstr(line, ".txt");
    assert(s != NULL);
    strncpy(s, ".dat", 4);
  }
  id_bin = open(line, O_RDONLY);
  if (id_bin < 0){
    perror(line);
    return 1;
  }
  if (fstat(id_bin, &buf) < 0){
    perror(line);
    return 1;
  }

  size_expected  = ((u_int64_t)npts_evt[0] * nch[0] + 4) * nevt_tot[0] * 2;
  size_expected += ((u_int64_t)npts_evt[1] * nch[1] + 4) * nevt_tot[1] * 2;
  if (buf.st_size != size_expected){
    printf("Binary data file size %ld bytes while expecting %ld bytes\n",
	   buf.st_size, size_expected);
    exit(1);
  }
  return 0;
}

int getEvent(int evtnum){
  int iseq;
  size_t offst;
  size_t eventSize0;
  size_t eventSize1;

  if (evtnum < 0 || evtnum >= nevt_tot[0]) return -1;

  eventSize0 = npts_evt[0] * nch[0] * sizeof(short) + sizeof(double);
  eventSize1 = npts_evt[1] * nch[1] * sizeof(short) + sizeof(double);
  offst = 0;

  /* Skipping all spills/sequences not containing requested event */
  for(iseq=0; evtnum >= nevt_seq[0][iseq]; iseq++){
    offst  += nevt_seq[0][iseq] * eventSize0 + nevt_seq[1][iseq] * eventSize1;
    evtnum -= nevt_seq[0][iseq];
  }

  /* Current sequence number */
  iseq_current = iseq;

  /* Event number within sequence */
  evtnum_seq = evtnum;

  if (offst != offst_current + offst_align){
    int ier;

    if (size != 0){
      munmap(file_ptr, size);
      adc_data = NULL;
    }

    /* Align to page boundary for mmap */
    offst_align   = offst & 0xfff;
    offst_current = offst - offst_align;
    size = offst_align
      + nevt_seq[0][iseq_current] * eventSize0
      + nevt_seq[1][iseq_current] * eventSize1;
    file_ptr = (caddr_t) mmap(NULL, size, PROT_READ, MAP_SHARED, id_bin, offst_current);
    if (file_ptr == NULL){
      perror("mmap()");
      exit(1);
    }
    ier = madvise(file_ptr, size, MADV_SEQUENTIAL | MADV_WILLNEED);
    if (ier < 0)
      perror("madvise()");
    adc_data = (int16_t *)(file_ptr + offst_align);
  }
  return 0;
}

int getAdcData(int chnum, float* buf, int maxpts)
{
  int offst;
  int i;
  int snum;
  if (chnum < 0 || chnum >= nch[0] + nch[1]) return -1;

  if (chnum < nch[0]){
    snum   = 0;
    offst  = npts_evt[0] * (chnum * nevt_seq[0][iseq_current] + evtnum_seq);
  }
  else{
    snum   = 1;
    chnum -= nch[0];
    /* Possibly the small scope may not have the requested event */
    if (evtnum_seq >= nevt_seq[1][iseq_current]) return 0;
    offst  = npts_evt[0] * nch[0] * nevt_seq[0][iseq_current];
    offst += npts_evt[1] * (chnum * nevt_seq[1][iseq_current] + evtnum_seq);
  }
  if (maxpts > npts_evt[snum]) maxpts = npts_evt[snum];
  for(i=0; i<maxpts; i++)
    buf[i] = adc_data[offst+i] * yscale[snum][chnum] + yoffst[snum][chnum];
  return maxpts;
}


#define NSMAX 16384

int main(int argc, char** argv){
  int ier, ievt, ich, ns, i, ipk;
  float buf[8][NSMAX];
  float v[8];
  float t[8];

  if (argc < 2) return 1;

  ier = scope_data_init(argv[1]);
  if (ier != 0) return 1;

  printf("#nch=%d+%d nevt=%d (%d)\n", nch[0], nch[1], nevt_tot[0], nevt_tot[1]);

  for(ievt=0; ievt<99999; ievt++){

    ier = getEvent(ievt);
    if (ier) break;

    printf("%8d", ievt);

    for(ich=0; ich<nch[0]+nch[1]; ich++){
      ipk = 0;
      v[ich] = 1E30;
      ns = getAdcData(ich, buf[ich], 16384);
      for (i=0; i<ns; i++)
	if (buf[ich][i] < v[ich]){
	  v[ich] = buf[ich][i];
	  ipk = i;
	}

      for(i=ipk-200; i<ipk; i++)
	if (i >= 0 && buf[ich][i] < 0.5 * v[ich]) break;

      if (i < ipk){
	t[ich] = i - (buf[ich][i] - 0.5 * v[ich]) / (buf[ich][i] - buf[ich][i-1]);
      }
      else
	t[ich] = -1;
      
      printf(" %e %.1f", v[ich], t[ich]);
    }
    putchar('\n');
  }
  return 0;
}
