#include "eudaq/Utils.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>

#ifndef WIN32

#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
typedef int SOCKET;
#endif

using eudaq::to_string;

// the port client will be connecting to
#define PORT_CONFIG 49248
#define PORT_DATATRANSF 49250
// max number of bytes we can get at once
#define MAXDATASIZE 300
#define MAXHOSTNAME 80


struct config_details_for_one_scope_type {
	std::string IPaddr;
	unsigned int Port;
 };
  
class KeysightScopeController {

public:
	KeysightScopeController();
	void SetConfigForScope(const config_details_for_one_scope_type conf);
	void SetAddress(std::string address);
	void SetPort(std::string port);
	int Write(char* buf);
	int Write(std::string command);
	int Read(char* buf);
	int Read(std::string& answer);
	std::string GetIdentification();
        void OpenConnection();
	void CloseConnection();
	virtual ~KeysightScopeController();

private:
  struct hostent *hclient, *hconfig, *hdatatransport;
  struct sockaddr_in client;
  struct sockaddr_in config;
  struct sockaddr_in datatransport;

  unsigned char conf_parameters[10];

  char ThisHost[80];
  std::string m_ScopeAddress;
  std::string m_config_socket_port;

  char Buffer_data[7000];
  char Buffer_length[7000];

  uint32_t data_trans_addres; // = INADDR_NONE;
  SOCKET sock_config;
  SOCKET sock_datatransport;
  int numbytes;

  // NiIPaddr;

};
