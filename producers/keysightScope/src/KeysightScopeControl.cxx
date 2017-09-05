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
  eudaq::OptionFlag get_iden(
      op, "i", "iden",
      "Get identification");	
  eudaq::Option<float> set_aux_voltage(
      op, "a", "set_aux_voltage",-99, "set_aux_voltage",
      "Set aux voltage level");		
  eudaq::OptionFlag get_active_channels(
      op, "ac", "active-channels",
      "Get active channels");
  eudaq::OptionFlag get_waveform_preamble(
      op, "gwp", "get-waveform-preamble",
      "Print waveform preamble for all channels");	
  eudaq::OptionFlag set_standard_settings(
      op, "ss", "set-standard_settings",
      "Set standard settings");	
  eudaq::OptionFlag get_status(
      op, "gs", "get-status",
      "Get status");	      
  eudaq::OptionFlag get_last_segment(
      op, "lcs", "last-captured-segment",
      "Get index of last segment");      

  try {
    op.Parse(argv);

    std::cout << "Using options:" << std::endl; 
    std::cout << "Host Name: " << host_name.Value() << std::endl;

    signal(SIGINT, ctrlchandler);

    KeysightScopeController SCOPE;
    SCOPE.SetAddress(host_name.Value());
    SCOPE.SetPort(host_port.Value());
    SCOPE.OpenConnection();
	  
     if(set_standard_settings.IsSet()) {
	     SCOPE.SetStandardSettings();
     }
     if(get_iden.IsSet()) {
	  std::cout << SCOPE.GetIdentification() << std::endl;
     };
     if(set_aux_voltage.Value()!=-99) {
	     std::cout << "Setting AUX to " << set_aux_voltage.Value() << " V" << std::endl;
	     SCOPE.SetAuxVoltage(set_aux_voltage.Value());
     };
     std::cout << "AUX: " << SCOPE.GetAuxStatus() << std::endl;
     if(get_active_channels.IsSet()) {
	  for(int i=0;i<4;i++){
		std::cout << "Channel " << i+1 << " active: " << SCOPE.IsChannelActive(i) << std::endl;
	  }
     };     
     if(get_waveform_preamble.IsSet()) {
	  for(int i=0;i<4;i++){
		std::cout << SCOPE.GetWaveformPreamble(i) << std::endl;
	  }	     
     }
     if(get_status.IsSet()) {
	  std::cout << SCOPE.GetStatus() << std::endl;
     }     
     if(get_last_segment.IsSet()) {
	     std::cout << SCOPE.GetCurrentSegmentIndex() << std::endl;
     }
    //SCOPE.SetVersion(fwver.Value());
    //SCOPE.Configure();
    SCOPE.CloseConnection();

    //std::cout << "Scope IDN = " << SCOPE.GetIdentifier() << std::endl;

 /*   if (quit.IsSet())
      return 0;
    eudaq::Timer totaltime, lasttime;

    if (pause.IsSet()) {
      std::cerr << "Press enter to start triggers." << std::endl;
      std::getchar();
    }
    TLU.Start();
    std::cout << "TLU Started!" << std::endl;

    uint32_t total = 0;
    while (!g_done) {
      TLU.Update(!nots.IsSet());
      std::cout << std::endl;
      TLU.Print(!nots.IsSet());
      if (sfile.get()) {
        for (size_t i = 0; i < TLU.NumEntries(); ++i) {
          *sfile << TLU.GetEntry(i).Eventnum() << "\t"
                 << TLU.GetEntry(i).Timestamp() << std::endl;
        }
      }
      total += TLU.NumEntries();
      double hertz = TLU.NumEntries() / lasttime.Seconds();
      double avghertz = total / totaltime.Seconds();
      lasttime.Restart();
      std::cout << "Time: " << totaltime.Formatted(TIME_FORMAT) << " s, "
                << "Freq: " << hertz << " Hz, "
                << "Average: " << avghertz << " Hz" << std::endl;
      if (wait.Value() > 0) {
        eudaq::mSleep(wait.Value());
      }
    }
    std::cout << "Quitting..." << std::endl;
    TLU.Stop();
    TLU.Update();
    // TLU.Print();
    */
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
