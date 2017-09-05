#include "../include/KeysightScopeController.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"

#ifndef WIN32
#include <unistd.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <csignal>
#include <memory>

//using namespace keysightScope;
using eudaq::to_string;
using eudaq::hexdec;

static sig_atomic_t g_done = 0;
#ifdef WIN32
static const std::string TIME_FORMAT = "%M:%S.%3";
#else
static const std::string TIME_FORMAT = "%s.%3";
#endif

void ctrlchandler(int) { g_done += 1; }

int main(int /*argc*/, char **argv) {

  eudaq::OptionParser op(
      "Keysight Scope Control Utility", "1.0",
      "A comand-line tool for controlling Keysight scopes");
	
  eudaq::Option<std::string> host_name(
      op, "r", "hostname", "", "127.0.0.1",
      "The host name if resolvable or the ip of the scope)");
  eudaq::Option<std::string> host_port(
      op, "p", "port", "5025", "port",
      "Port number");	
  eudaq::Option<float> set_aux_voltage(
      op, "a", "set_aux_voltage",-99, "set_aux_voltage",
      "Set aux voltage level");		
  eudaq::OptionFlag do_simple_handshake(
      op, "hs", "handshake",
      "Do simple handshake");      

  try {
    op.Parse(argv);

    KeysightScopeController SCOPE;
    SCOPE.SetAddress(host_name.Value());
    SCOPE.SetPort(host_port.Value());
    SCOPE.OpenConnection();
	  
     if(set_aux_voltage.Value()!=-99) {
	     std::cout << "Setting AUX to " << set_aux_voltage.Value() << " V" << std::endl;
	     SCOPE.SetAuxVoltage(set_aux_voltage.Value());
     };
     
     if(do_simple_handshake.IsSet()) {
	     SCOPE.SetAuxVoltage(1.5);
	     SCOPE.SetAuxVoltage(0);
     }
    SCOPE.CloseConnection();

  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
