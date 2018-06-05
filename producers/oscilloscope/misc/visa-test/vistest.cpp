#include <visa.h>
#include <iostream>
#include <stdio.h>

int main () 
{
	ViSession rm;
	ViPSession io=VI_NULL;
	ViInt32 ret = VI_ERROR_SYSTEM_ERROR;
	
	const char resource[] = "TCPIP0::192.168.4.246::hislip0,4880::INSTR";
	
	#define BUFFER_SIZE 4096
	char rd_buffer[BUFFER_SIZE];

	//open the local VISA resource manager
	if(VI_SUCCESS != viOpenDefaultRM(&rm))
	{
		printf("\nOpen VISA resource manager failed!\n");
		return VI_ERROR_SYSTEM_ERROR;
	}
		
	ret=viOpen(rm,resource,VI_NULL,VI_NULL,io);
	if(ret!= VI_SUCCESS) {
		std::cout << "error connecting: " << ret << std::endl;
		return ret;
	}
	
	ret=viLock(*io, VI_EXCLUSIVE_LOCK, 0, "", VI_NULL);
	if(ret!=VI_SUCCESS)
	{
		printf("Exclusiv locking of VISA resource failed!\n");
		return ret;
	}	
	
	viQueryf(*io, "IDN?\n", "%s", rd_buffer);
	
	std::cout << rd_buffer << std::endl;
}
