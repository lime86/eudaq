#include "KeysightScopeController.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include <stdio.h>
#include <stdlib.h>
#include <memory>

using eudaq::RawDataEvent;

class KeysightScopeProducer : public eudaq::Producer {
public:
  KeysightScopeProducer(const std::string &runcontrol)
      : eudaq::Producer("KeysightScope", runcontrol), done(false), running(false),
        stopping(false) {

    configure = false;

    std::cout << "Keysight Scope Producer was started successful " << std::endl;
  }
  void MainLoop() {
    EUDAQ_DEBUG("Entering main loop of scope producer");
    do {
      if (!running) {
        eudaq::mSleep(50);
        continue;
      }
      if (running) {
		bool start_readout = false;
		/* Wait for ADONE other states are ARM | TRIG | ATRIG  or for recorded number of segments to go above 
		threshold in order to leave a bit of time for incoming trigger - safer as we do no have a service request! */
	        //EUDAQ_DEBUG("Running");
		if (scope_control[0]->GetStatus().at(1) == 'D'){
		      start_readout = true;
		      std::cout << "We reached ADONE - decrease threshold!" << std::endl;
		      EUDAQ_DEBUG("We reached ADONE - decrease threshold!");
		}
		eudaq::mSleep(50);
		if(scope_control[0]->GetCurrentSegmentIndex()>=7000) {
			start_readout = true;
			EUDAQ_DEBUG("We reached 20000 segments");
		}
		
		if(start_readout) {
			EUDAQ_DEBUG("Scope producer entering readout");
			if(scope_control[0]->GetCurrentSegmentIndex()<=0) continue;
			std::cout << "Received " << scope_control[0]->GetCurrentSegmentIndex() << " segments" << std::endl;
			
			// prepare for stop of triggers
			scope_control[0]->SetAuxVoltage(-0.9);
			eudaq::mSleep(1); //adjust this value by trial and error
			scope_control[0]->StopRun();
			
			//Number of events in slave scope is UNKNOWN yet 
			int nsegm1 = 0;
			for(unsigned int current_scope_id = 0; current_scope_id < NumberOfScopes; current_scope_id++){
				/* --- Read waveform data for all selected channels */
				for(unsigned int current_channel=0; current_channel<4; current_channel++){
					if(config_details_for_all_scopes[current_scope_id].channelEnabled[current_channel]==false) continue;
					
				}
			}
			scope_control[0]->SetAuxVoltage(0);
		}
	}
		
    } while (!done);
  }
  virtual void OnInitialise(const eudaq::Configuration &init) {
        std::cout << "... was Initialized " << init.Name() << " " << std::endl;
        EUDAQ_INFO("Initialised (" + init.Name() + ")");
        SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");	  
  }	  
  virtual void OnConfigure(const eudaq::Configuration &param) {
	  
	KeysightConfigWithErrors = true;
	try {
		unsigned char configur[5] = "conf";
		
		NumberOfScopes = param.Get("NumberOfScopes", 1);
		std::cout << "Number of scopes: " << NumberOfScopes << std::endl;
		for(unsigned int current_scope_id = 0; current_scope_id < NumberOfScopes; current_scope_id++) {
			config_details_for_one_scope_type config_details_for_one_scope;
			config_details_for_all_scopes.push_back(config_details_for_one_scope);
			config_details_for_all_scopes[current_scope_id].IPaddr = param.Get("Scope[" + to_string(current_scope_id) + "].IPaddr", "invalid");
			config_details_for_all_scopes[current_scope_id].Port = param.Get("Scope[" + to_string(current_scope_id) + "].Port", 32767);
			if(config_details_for_all_scopes[current_scope_id].IPaddr=="invalid") {
				std::cout << "Producer " << param.Name() << ": IP not set for scope " << current_scope_id << std::endl;
				EUDAQ_INFO("IP not set for scope " + current_scope_id);
				SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "IP not set for scope " + current_scope_id);
			};
		}

		for(unsigned int current_scope_id = 0; current_scope_id < NumberOfScopes; current_scope_id++) {
			std::cout << "Scope " << current_scope_id << " has IP address " << config_details_for_all_scopes[current_scope_id].IPaddr << std::endl;
			std::cout << "Scope " << current_scope_id << " has port number " << config_details_for_all_scopes[current_scope_id].Port << std::endl;
			scope_control.push_back(std::make_shared<KeysightScopeController>() );
			scope_control[current_scope_id]->SetAddress(config_details_for_all_scopes[current_scope_id].IPaddr);
			scope_control[current_scope_id]->SetPort(std::to_string(config_details_for_all_scopes[current_scope_id].Port));
			scope_control[current_scope_id]->OpenConnection();
			scope_control[current_scope_id]->SetAuxVoltage(0);
			scope_control[current_scope_id]->SetStandardSettings();
			//scope_control->ConfigClientSocket_Open(param);
			//scope_control->DatatransportClientSocket_Open(param);
	
			for(int i=0;i<4;i++){
				config_details_for_all_scopes[current_scope_id].channelEnabled[i] = scope_control[current_scope_id]->IsChannelActive(i);
			};
		}		
		
		std::cout << "Configuring ...(" << param.Name() << ")" << std::endl;

		//if (!configure) {
		//scope_control.push_back(std::make_shared<KeysightScopeController>() );
		//scope_control->GetProducerHostInfo();
		//scope_control->ConfigClientSocket_Open(param);
		//scope_control->DatatransportClientSocket_Open(param);
		std::cout << " " << std::endl;
		configure = true;
		KeysightConfigWithErrors = false;

      if (KeysightConfigWithErrors) {
        std::cout << "NI crate was Configured with ERRORs " << param.Name()
                  << " " << std::endl;
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
      } else {
        std::cout << "... was Configured " << param.Name() << " " << std::endl;
        EUDAQ_INFO("Configured (" + param.Name() + ")");
        SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
      }
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
    }
  }
  virtual void OnStartRun(unsigned param) {
    try {
	    m_run = param;
	    EUDAQ_DEBUG("Starting Keysight scope run");
	    eudaq::RawDataEvent ev(RawDataEvent::BORE("KeyScope", m_run));
	    ev.SetTag("NumberOfScopes", NumberOfScopes);
	    EUDAQ_DEBUG("BORE created for Keysight scope");
	    
		for(unsigned int current_scope_id = 0; current_scope_id < NumberOfScopes; current_scope_id++) {
			EUDAQ_DEBUG("Dealing with scope " + std::to_string(current_scope_id));
			for(unsigned int current_channel=0; current_channel<4; current_channel++){\
				EUDAQ_DEBUG("Dealing with channel " + std::to_string(current_channel));
				if(config_details_for_all_scopes[current_scope_id].channelEnabled[current_channel]==false) continue;

				/* Waveform PREamble first and put in the BORE */
				EUDAQ_DEBUG("Getting preamble and writing into BORE as tag");
				ev.SetTag("Preamble_Scope_" + std::to_string(current_scope_id) + "_Channel_" + std::to_string(current_channel), scope_control[current_scope_id]->GetWaveformPreamble(current_channel));
				}	
			EUDAQ_DEBUG("Arming scope " + std::to_string(current_scope_id));				
			scope_control[current_scope_id]->StartRun();
		}		
		running = true;
		EUDAQ_DEBUG("Removing veto scope 0");	
		SendEvent(ev);
		scope_control[0]->SetAuxVoltage(0);
		SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Started");
		EUDAQ_DEBUG("Scope producer connection state running");	
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Start Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Start Error");
    }
    EUDAQ_DEBUG("Scope producer leaving on start run");	
  }
  virtual void OnStopRun() {
    try {
      std::cout << "Stop Run" << std::endl;

	for(unsigned int current_scope_id = 0; current_scope_id < NumberOfScopes; current_scope_id++) {
		scope_control[current_scope_id]->StopRun();
	}	
      running = false;
      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
    }
  }
  virtual void OnTerminate() {
    std::cout << "Terminate (press enter)" << std::endl;
    done = true;
 	for(unsigned int current_scope_id = 0; current_scope_id < NumberOfScopes; current_scope_id++) {
		scope_control[current_scope_id]->CloseConnection();
	}	   
    eudaq::mSleep(1000);
  }

private:
  unsigned m_run, m_ev;
  bool done, running, stopping, configure;
  struct timeval tv;
  std::vector<std::shared_ptr<KeysightScopeController>> scope_control;

  char *Buffer1;
  unsigned int datalength1;
  char *Buffer2;
  unsigned int datalength2;
  unsigned int ConfDataLength;
  std::vector<unsigned char> ConfDataError;
  int r;
  int t;

  unsigned int StatusLine;
  unsigned int OVF;
  unsigned int NumOfState;
  unsigned int AddLine;
  unsigned int State;
  unsigned int AddColum;
  unsigned int HitPix;

  unsigned int data[7000];
  unsigned char NumOfDetector;
  unsigned int NumOfData;
  unsigned int NumOfLengt;
  unsigned int MIMOSA_DatalenTmp;
  int datalengthAll;

  unsigned int NumberOfScopes;
  std::vector<config_details_for_one_scope_type> config_details_for_all_scopes;
  bool OneFrame;
  bool KeysightConfigWithErrors;

  unsigned char conf_parameters[10];
};
// ----------------------------------------------------------------------
int main(int /*argc*/, const char **argv) {
  std::cout << "Start Producer \n" << std::endl;

  eudaq::OptionParser op("EUDAQ Keysight Scope Producer", "0.1",
                         "The Producer task for one or more Keysight Scopes");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "KeysightScope", "string",
                                  "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    KeysightScopeProducer producer(rctrl.Value());
    producer.MainLoop();
    eudaq::mSleep(500);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
