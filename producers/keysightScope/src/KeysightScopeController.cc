#include "KeysightScopeController.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/types.h>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define EUDAQ_BCOPY(source, dest, NSize) bcopy(source, dest, NSize)
#define EUDAQ_Sleep(x) sleep(x)
#define EUDAQ_CLOSE_SOCKET(x) close(x)
#else

#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
#define EUDAQ_Sleep(x) Sleep(x)
#define EUDAQ_BCOPY(source, dest, NSize) memmove(dest, source, NSize)

#define EUDAQ_CLOSE_SOCKET(x) closesocket(x)

#endif

int EUDAQ_SEND(SOCKET s, unsigned char *buf, int len, int flags) {

#ifdef WIN32
  // on windows this function takes a signed char ...
  // in prinziple I always distrust reinterpreter_cast but in this case it
  // should be save
  return send(s, reinterpret_cast<const char *>(buf), len, flags);

#else
  return send(s, buf, len, flags);
#endif
}

unsigned char start[5] = "star";
unsigned char stop[5] = "stop";

KeysightScopeController::KeysightScopeController() {
  // NI_IP = "192.76.172.199";
}

void KeysightScopeController::SetConfigForScope(const config_details_for_one_scope_type conf) {
	
}

void KeysightScopeController::SetAddress(std::string address) {
	m_ScopeAddress = address;
}

void KeysightScopeController::SetPort(std::string port) {
	m_config_socket_port = port;
}

std::string KeysightScopeController::GetIdentification() {
	
}

void KeysightScopeController::OpenConnection()
{
	
  // convert string in config into IPv4 address
  hostent *host = gethostbyname(m_ScopeAddress.c_str());
  if (!host) {
    EUDAQ_ERROR("ConfSocket: Bad Scope IPaddr value in config file: must be legal "
                "IPv4 address!");
    perror("ConfSocket: Bad Scope IPaddr value in config file: must be legal IPv4 "
           "address: ");
  }
  
  if ((sock_config = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    EUDAQ_ERROR("ConfSocket: Error creating the TCP socket  ");
    perror("ConfSocket Error: socket()");
    exit(1);
  } else {
	  printf("----TCP/Scope: SOCKET is OK...\n");
  }

  /* --- connect to the scope */
  
  /*sa_scope[0].sin_addr = *((struct in_addr *)he->h_addr);
  bzero(sa_scope[0].sin_zero, 8);
  if (connect(sock_config, (struct sockaddr *)&sa_scope[0],
	      sizeof(struct sockaddr)) == -1){
    perror(HOST);
    exit(1);
  }*/
  
  memcpy((char *)&config.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
  
  printf("----TCP/Scopa INET ADDRESS is: %s \n", inet_ntoa(config.sin_addr));
  printf("----TCP/Scope INET PORT is: %s \n", m_config_socket_port.c_str());

  config.sin_family = AF_INET;
  int i_auto = std::stoi(m_config_socket_port, nullptr, 10);
  config.sin_port = htons(i_auto);
  memset(&(config.sin_zero), '\0', 8);
  if (connect(sock_config, (struct sockaddr *)&config,
              sizeof(struct sockaddr)) == -1) {
    EUDAQ_ERROR("ConfSocket: Scope doesn't appear to be "
                "running  ");
    perror("ConfSocket Error: connect()");
    EUDAQ_Sleep(60);
    exit(1);
  } else
    printf("----TCP/Scope The CONNECT is OK...\n");
  
}

void KeysightScopeController::CloseConnection() {
  EUDAQ_CLOSE_SOCKET(sock_config);
}

KeysightScopeController::~KeysightScopeController() {
  CloseConnection();
}
