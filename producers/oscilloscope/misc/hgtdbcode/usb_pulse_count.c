#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

static int m_fd;
static unsigned char mcp_buf[256];

int mcp_init(){
  int ier;

  m_fd = open("/dev/hidraw3", O_RDWR);
  if (m_fd < 0){
    perror("mcp_init: Cannot open USB device special file");
    return 1;
  }

  ier = ioctl(m_fd, HIDIOCGRAWNAME(256), mcp_buf);
  if (ier < 0){
    perror("mcp_init: Cannot get USB device name");
    return 1;
  }

  fprintf(stderr, "mcp_init: Found %s\n", mcp_buf);

  memset(mcp_buf, 0, 64);
  mcp_buf[0] = 0x20;
  ier = write(m_fd, mcp_buf, 64);
  if (ier < 0) goto exc;
  ier = read(m_fd, mcp_buf, 64);
  if (ier < 0) goto exc;
  if (mcp_buf[0] != 0x20 || mcp_buf[1] != 0) goto exc;
  mcp_buf[0]   = 0x21;
  mcp_buf[1]   = mcp_buf[2] = mcp_buf[3] = 0;
  mcp_buf[10]  = 2;
  mcp_buf[17] &= 0xf1;
  mcp_buf[17] |= 0x04;
  mcp_buf[18]  = 0;
  ier = write(m_fd, mcp_buf, 64);
  if (ier < 0) goto exc;
  ier = read(m_fd, mcp_buf, 64);
  if (ier < 0) goto exc;
  if (mcp_buf[0] != 0x21 || mcp_buf[1] != 0) goto exc;
  return 0;
 exc:
  fputs("mcp_init: Could not complete initialization\n", stderr);
  return 1;
}


int mcp_get_pulse_count(unsigned int* val){
  int ier;
  memset(mcp_buf, 0, 64);
  mcp_buf[0] = 0x12;
  mcp_buf[1] = 1;
  ier = write(m_fd, mcp_buf, 64);
  if (ier < 0) goto exc;
  ier = read(m_fd, mcp_buf, 64);
  if (ier < 0) goto exc;
  if (mcp_buf[0] != 0x12 || mcp_buf[1] != 0) goto exc;
  *val = mcp_buf[4] | mcp_buf[5] << 8;
  return 0;
 exc:
  return 1;
}
