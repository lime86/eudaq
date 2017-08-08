#include "KeysightScopeController.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

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
	Write(std::string("*IDN?\n"));
	return Read();
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

int KeysightScopeController::Write(char* buf){
  int val;
  int len = strlen(buf);
  if ( sock_config != 0){ //if scope?configured...
    val = send(sock_config, buf, len, 0);
    if (val <= 0){
      perror(buf);
      return 0;
    }
  }
  return 1;
}

int KeysightScopeController::Write(std::string command) {
    snprintf(buffer_command, BUFFER_OUT_SIZE, "%s" , command.c_str());
    std::cout << "Output command: " << command  << std::endl;
    return Write(buffer_command);
}

int KeysightScopeController::Read(char* buf){
    if (sock_config != 0){
      return recv(sock_config, buf, BUFFER_IN_SIZE, 0);
    } else return -1;
}

std::string KeysightScopeController::Read(){
	int operation_successful;
	std::string answer;
	
	operation_successful = Read(buffer_answer);
	answer.assign(buffer_answer,strlen(buffer_answer));
	//std::cout << "Received answer: " << buffer_answer << std::endl;
	std::cout << "Received answer: " << answer << std::endl;
	return answer;
}

int KeysightScopeController::SetAuxVoltage(float voltage){
	if((voltage<-2.4)||(voltage>2.4)) {
		std::cout << "Aux voltage outside of acceptable range" << std::endl;
		return -1;
	}
	sprintf(buffer_command,":CAL:OUTP DC,%.2f\n",voltage);
	//std::cout << buffer_command_string << std::endl;
	Write(buffer_command);
	return 0;
}

std::string KeysightScopeController::GetAuxStatus() {
	Write(std::string(":CAL:OUTP?\n"));
	return Read();
}

void KeysightScopeController::SetStandardSettings() {
	  if(Write(std::string("STOP\n"))<0)
		  std::cout << "Error issuing STOP" << std::endl;
	  if(Write(std::string(":SYST:HEAD 0\n"))<0)
		  std::cout << "Error issuing :SYST:HEAD 0" << std::endl;
	  if(Write(std::string(":WAV:FORM BIN;BYT LSBF;STR 1;SEGM:ALL 1\n"))<0)
		  std::cout << "Error issuing :WAV:FORM BIN;BYT LSBF;STR 1;SEGM:ALL 1" << std::endl;
	/* Make sure we turn off sin(x)/x interpolation SSIM 01-NOV-2016 */
	  if(Write(std::string(":ACQ:MODE SEGM;INT 0\n"))<0)
		  std::cout << "Error issuing :ACQ:MODE SEGM;INT 0" << std::endl;		  

  /* Slave scope specific setup */
  //swrite(":TIM:REFC 1\n",  1);
  //swrite(":TRIG:SWE TRIG\n",  1);
  //swrite(":TRIG:MODE EDGE\n", 1);
  //swrite(":TRIG:LEV AUX,0.5\n", 1);
  //swrite(":TRIG:EDGE:COUP DC;SLOP POS;SOUR AUX\n", 1);
}

bool KeysightScopeController::IsChannelActive(int channel) {
    snprintf(buffer_command, sizeof(buffer_command), ":STAT? CHAN%d\n", channel+1);
    Write(buffer_command);
    if(Read(buffer_answer)<0) {std::cout << "error reading active channels" << std::endl;}
    //std::cout << buffer_answer << std::endl;
    //std::cout << atoi(buffer_answer) << std::endl;
    return atoi(buffer_answer);
	    /*
      if (nbytes <= 0)
	goto the_end;
      if (atol(buf)){
	chmask |= (1 << i);
      }
    }
	  */
};

void KeysightScopeController::StartRun(){
	Write(std::string(":RUN\n"));
};

void KeysightScopeController::StopRun(){
	Write(std::string(":STOP\n"));
};

std::string KeysightScopeController::GetStatus(){
	Write(std::string(":AST?\n"));
	sleep(1);
	return Read();
};

int KeysightScopeController::GetCurrentSegmentIndex(){
	Write(std::string(":WAV:SEGM:COUN?\n"));
	sleep(1);
	Read(buffer_answer);
	return atoi(buffer_answer);
}

std::string KeysightScopeController::GetWaveformPreamble(unsigned int channel){
	snprintf(buffer_command, sizeof(buffer_command), ":WAV:SOUR CHAN%d;PRE?\n", channel + 1);
	Write(buffer_command);
	return Read();
}

preamble_data_type KeysightScopeController::DecodeWaveformPreamble(std::string preamble_string){
	std::stringstream ss(preamble_string);
	std::string token[24];
	
	int counter = 0;
	while(ss >> token[counter]) {
		counter++;
		if (ss.peek()==',')
			ss.ignore();
	}
	/*
	unsigned int type;
	long int points;
	unsigned int count;
	long int X_increment;
	double X_origin;
	long int X_reference;
	long int Y_increment;
	double Y_origin;
	long int Y_reference;
	unsigned int coupling;
	long int X_display_range;
	double X_display_origin;
	long int Y_display_range;
	double Y_display_origin;
	std::string date;
	std::string time;
	std::string frame_model_number;
	unsigned int acquisition_mode;
	int completion;
	unsigned int X_units;
	unsigned int Y_units;
	float max_bandwidth_limit;
	float min_bandwidth_limit;);*/
}

void KeysightScopeController::CloseConnection() {
  EUDAQ_CLOSE_SOCKET(sock_config);
}

KeysightScopeController::~KeysightScopeController() {
  CloseConnection();
}
