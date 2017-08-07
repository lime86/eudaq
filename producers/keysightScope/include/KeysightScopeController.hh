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

#define BUFFER_OUT_SIZE 16384
#define BUFFER_IN_SIZE 16384


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
	std::string Read();

	std::string GetIdentification();
	int SetAuxVoltage(float voltage);
	std::string GetAuxStatus();

	void SetStandardSettings();

	bool IsChannelActive(int channel);

        void OpenConnection();
	void CloseConnection();

	virtual ~KeysightScopeController();

private:
  struct sockaddr_in config;

  unsigned char conf_parameters[10];

  std::string m_ScopeAddress;
  std::string m_config_socket_port;

  char buffer_command[BUFFER_OUT_SIZE];
  char buffer_answer[BUFFER_IN_SIZE];

  SOCKET sock_config;

};
