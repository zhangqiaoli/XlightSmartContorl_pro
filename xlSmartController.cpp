/**
 * xlSmartController.cpp - contains the major implementation of SmartController.
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2016 DTIT
 * Full contributor list:
 *
 * Documentation:
 * Support Forum:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * DESCRIPTION
 * 1.
 *
 * ToDo:
**/
#include "xlSmartController.h"
#include "xliPinMap.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxPanel.h"
#include "xlxRF433Server.h"
#include "xlxSerialConsole.h"

//------------------------------------------------------------------
// Global Data Structures & Variables
//------------------------------------------------------------------
// make an instance for main program
SmartControllerClass theSys;

//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
SmartControllerClass::SmartControllerClass()
{
  m_isLAN = false;
  m_isWAN = false;
  m_isRF = false;
}

BOOL SmartControllerClass::IsLANGood()
{
	return m_isLAN;
}

BOOL SmartControllerClass::IsWANGood()
{
	return m_isWAN;
}

BOOL SmartControllerClass::IsRFGood()
{
	return (m_isRF && theRadio.isValid());
}

UC SmartControllerClass::GetStatus()
{
	return (UC)m_SysStatus;
}

BOOL SmartControllerClass::SetStatus(UC st)
{
	if( st > STATUS_ERR ) return false;
	LOGN(LOGTAG_STATUS, "System status changed from %d to %d", m_SysStatus, st);
	if ((UC)m_SysStatus != st) {
		m_SysStatus = st;
	}
	return true;
}

// Primitive initialization before loading configuration
void SmartControllerClass::Init()
{
  // Open Serial Port
  TheSerial.begin(SERIALPORT_SPEED_DEFAULT);

#ifdef SYS_SERIAL_DEBUG
	// Wait Serial connection so that we can see the starting information
	while(!TheSerial.available()) {
		if( Particle.connected() == true ) { Particle.process(); }
	}
	SERIAL_LN("SmartController is starting...");
#endif

	// Get System ID
	m_SysID = System.deviceID();
	m_SysVersion = System.version();
	m_SysStatus = STATUS_INIT;

	// Initialize Logger
	theLog.Init(m_SysID);
	theLog.InitFlash(MEM_OFFLINE_DATA_OFFSET, MEM_OFFLINE_DATA_LEN);

	LOGN(LOGTAG_MSG, "SmartController is starting...SysID=%s", m_SysID.c_str());
}

// Get the controller started
BOOL SmartControllerClass::Start()
{
	UC pre_relay_keys = theConfig.GetRelayKeys();

	LOGN(LOGTAG_MSG, "SmartController started.");
	LOGI(LOGTAG_MSG, "Product Info: %s-%s-%d",
			theConfig.GetOrganization().c_str(), theConfig.GetProductName().c_str(), theConfig.GetVersion());
	LOGI(LOGTAG_MSG, "System Info: %s-%s",
			GetSysID().c_str(), GetSysVersion().c_str());

#ifndef SYS_SERIAL_DEBUG
	ResetSerialPort();
#endif

	// Change Panel LED ring to the recent level
	thePanel.SetDimmerValue(theConfig.GetBrightIndicator());
  // TODO panel start status restore

	// TODO Restore relay key to previous state
	theConfig.SetRelayKeys(pre_relay_keys);
	//relay_restore_keystate();

	// Publish local the main device status
	if( Particle.connected() == true ) {
		UL tm = 0x4ff;	// Delay 1.5s in order to publish
		while(tm-- > 0){ Particle.process(); }
	}
	//TODO  current device restore
	//RequestDeviceStatus(CURRENT_DEVICE);

	return true;
}

void SmartControllerClass::Restart()
{
	theConfig.SaveConfig();
	SetStatus(STATUS_RST);
	delay(1000);
	System.reset();
}

// Initialize Pins: check the routine with PCB
void SmartControllerClass::InitPins()
{
#ifdef PIN_SOFT_KEY_1
	pinMode(PIN_SOFT_KEY_1, OUTPUT);
	//pinSetFast(PIN_SOFT_KEY_1);
#endif
#ifdef PIN_SOFT_KEY_4
	pinMode(PIN_SOFT_KEY_4, OUTPUT);
#endif
#ifdef PIN_SOFT_KEY_2
	pinMode(PIN_SOFT_KEY_2, OUTPUT);
#endif
#ifdef PIN_SOFT_KEY_3
	pinMode(PIN_SOFT_KEY_3, OUTPUT);
#endif

#ifdef EN_BTN_EXT_1
	btnExt1.debounceTime   = 30;   // Debounce timer in ms
	btnExt1.multiclickTime = 500;  // Time limit for multi clicks
	btnExt1.longClickTime  = 2000; // time until "held-down clicks" register
#endif

#ifdef EN_BTN_EXT_2
	btnExt2.debounceTime   = 30;   // Debounce timer in ms
	btnExt2.multiclickTime = 500;  // Time limit for multi clicks
	btnExt2.longClickTime  = 2000; // time until "held-down clicks" register
#endif

#ifdef EN_BTN_EXT_3
	btnExt3.debounceTime   = 30;   // Debounce timer in ms
	btnExt3.multiclickTime = 500;  // Time limit for multi clicks
	btnExt3.longClickTime  = 2000; // time until "held-down clicks" register
#endif

#ifdef EN_BTN_EXT_4
	btnExt4.debounceTime   = 30;   // Debounce timer in ms
	btnExt4.multiclickTime = 500;  // Time limit for multi clicks
	btnExt4.longClickTime  = 2000; // time until "held-down clicks" register
#endif

	// Init Panel components
	thePanel.InitPanel();
	// Change Panel LED ring to indicate panel is working
	thePanel.CheckLEDRing(2);
	thePanel.CheckLEDRing(3);
}

void SmartControllerClass::InitRadio()
{
  m_isRF = theRadio.ServerBegin(theConfig.GetRFChannel(),theConfig.GetRFChannel());
  if (m_isRF)
  {
    LOGN(LOGTAG_MSG, "RF2.4 is working.");
    SetStatus(STATUS_BMW);
  }
  else
  {
    LOGN(LOGTAG_MSG, "RF2.4 is not working.");
  }
}

void SmartControllerClass::InitCloudObj()
{
	// Set cloud variable initial value
	m_tzString = theConfig.GetTimeZoneJSON();

	if( theConfig.GetUseCloud() != CLOUD_DISABLE ) {
		CloudObjClass::InitCloudObj();
		LOGN(LOGTAG_MSG, "Cloud Objects registered.");
	}
}


// Third level initialization after loading configuration
/// check LAN & WAN
void SmartControllerClass::InitNetwork()
{
	BOOL oldWAN = IsWANGood();
	BOOL oldLAN = IsLANGood();

	// Check WAN and LAN
	CheckNetwork();

	if (IsWANGood())
	{
		if( !oldWAN ) {	// Only log when status changed
			LOGN(LOGTAG_EVENT, "WAN is working.");
			SetStatus(STATUS_NWS);
		}
	}
	else if (IsLANGood())
	{
		if( !oldLAN ) {	// Only log when status changed
			LOGN(LOGTAG_EVENT, "LAN is working.");
			SetStatus(STATUS_DIS);
		}
	}
	else if (IsRFGood()) {
		SetStatus(STATUS_BMW);
	}
	else {
		SetStatus(STATUS_ERR);
	}
}


// Check Wi-Fi module and connection
BOOL SmartControllerClass::CheckWiFi()
{
	if( theConfig.GetDisableWiFi() ) return false;

	if( WiFi.RSSI() > 0 ) {
		m_isWAN = false;
		m_isLAN = false;
		LOGE(LOGTAG_MSG, "Wi-Fi chip error!");
		return false;
	}

	if( !WiFi.ready() ) {
		m_isWAN = false;
		m_isLAN = false;
		LOGE(LOGTAG_EVENT, "Wi-Fi module error!");
		return false;
	}
	return true;
}

BOOL SmartControllerClass::CheckNetwork()
{
	// Check Wi-Fi module
	if( !CheckWiFi() ) {
		return false;
	}

	// Check WAN
	m_isWAN = WiFi.resolve("www.google.com");

	// Check LAN if WAN is not OK
	if( !m_isWAN ) {
		m_isLAN = (WiFi.ping(WiFi.gatewayIP(), 3) < 3);
		if( !m_isLAN ) {
			LOGW(LOGTAG_MSG, "Cannot reach local gateway!");
			m_isLAN = (WiFi.ping(WiFi.localIP(), 3) < 3);
			if( !m_isLAN ) {
				LOGE(LOGTAG_MSG, "Cannot reach itself!");
			}
		}
	} else {
		m_isLAN = true;
	}

	return true;
}

// Connect Wi-Fi
BOOL SmartControllerClass::connectWiFi()
{
	if( theConfig.GetDisableWiFi() ) return false;

	BOOL retVal = WiFi.ready();
	if( !retVal ) {
		if( WiFi.hasCredentials() ) {
			SERIAL("Wi-Fi connecting...");
		  WiFi.connect();
		  waitFor(WiFi.ready, RTE_WIFI_CONN_TIMEOUT);
		  retVal = WiFi.ready();
			SERIAL_LN("%s", retVal ? "OK" : "Failed");
		}
	}
  theConfig.SetWiFiStatus(retVal);
  return retVal;
}

// Connect to the Cloud
BOOL SmartControllerClass::connectCloud()
{
	BOOL retVal = Particle.connected();
	if( !retVal ) {
		SERIAL("Cloud connecting...");
	  Particle.connect();
	  waitFor(Particle.connected, RTE_CLOUD_CONN_TIMEOUT);
	  retVal = Particle.connected();
		SERIAL_LN("%s", retVal ? "OK" : "Failed");
	}
  return retVal;
}

int SmartControllerClass::ExeJSONCommand(String jsonCmd)
{
	SERIAL_LN("Execute JSON cmd: %s", jsonCmd.c_str());

	int rc = ProcessJSONString(jsonCmd);
	if (rc < 0) {
		// Error input
		LOGE(LOGTAG_MSG, "Error parsing json cmd: %s", jsonCmd.c_str());
		return 0;
	} else if (rc > 0) {
		// Wait for more...
		return 1;
	}

	int sub_id = 0;
	if( (*m_jpCldCmd).containsKey("sid") ) sub_id = (*m_jpCldCmd)["sid"].as<int>();

	rc = 0;
	/*if ((*m_jpCldCmd).containsKey("cmd"))
  {
    const COMMAND _cmd = (COMMAND)(*m_jpCldCmd)["cmd"].as<int>();
    //COMMAND 0: Use Serial Interface
    if( _cmd == CMD_SERIAL) {
      // Execute serial port command, and reflect results on cloud variable
      if ((*m_jpCldCmd).containsKey("data")) {
        if( !theConfig.IsCloudSerialEnabled() ) {
          LOGN(LOGTAG_MSG, "Cloud serial command is not allowed. Check system config.");
          return 0;
        }
        const char* strData = (*m_jpCldCmd)["data"];
        theConsole.ExecuteCloudCommand(strData);
        return 1;
      }
    }
    //COMMAND 1: Toggle light switch
    else if (_cmd == CMD_POWER) {
      if ((*m_jpCldCmd).containsKey("nd") && (*m_jpCldCmd).containsKey("state")) {
        const int node_id = (*m_jpCldCmd)["nd"].as<int>();
        const int state = (*m_jpCldCmd)["state"].as<int>();
        const UC _hwsw = (UC)((*m_jpCldCmd).containsKey("hw") ? (*m_jpCldCmd)["hw"].as<int>() : 2);
        return DeviceSwitch(state, _hwsw, node_id, sub_id);
      }
    }
    //COMMAND 2: Change light color
		else if (_cmd == CMD_COLOR) {
			if ((*m_jpCldCmd).containsKey("nd") && (*m_jpCldCmd).containsKey("ring")) {
				if( (*m_jpCldCmd)["ring"].size() > 6 ) {
					const int node_id = (*m_jpCldCmd)["nd"].as<int>();
					const uint8_t ring = (*m_jpCldCmd)["ring"][0].as<uint8_t>();
					const uint8_t State = (*m_jpCldCmd)["ring"][1].as<uint8_t>();
					const uint8_t BR = (*m_jpCldCmd)["ring"][2].as<uint8_t>();
					const uint8_t W = (*m_jpCldCmd)["ring"][3].as<uint8_t>();
					const uint8_t R = (*m_jpCldCmd)["ring"][4].as<uint8_t>();
					const uint8_t G = (*m_jpCldCmd)["ring"][5].as<uint8_t>();
					const uint8_t B = (*m_jpCldCmd)["ring"][6].as<uint8_t>();

					MyMessage tmpMsg;
					UC payl_buf[MAX_PAYLOAD];
					UC payl_len;

					payl_len = CreateColorPayload(payl_buf, ring, State, BR, W, R, G, B);
					tmpMsg.build(theRadio.getAddress(), node_id, sub_id, C_SET, V_RGBW, true);
					tmpMsg.set((void *)payl_buf, payl_len);
					return theRadio.ProcessSend(&tmpMsg);
				}
			}
		}
		//COMMAND 3: Change brightness
		//COMMAND 5: Change CCT
		else if (_cmd == CMD_BRIGHTNESS || _cmd == CMD_CCT) {
			if ((*m_jpCldCmd).containsKey("nd") && (*m_jpCldCmd).containsKey("value")) {
				const int node_id = (*m_jpCldCmd)["nd"].as<int>();
				const int value = (*m_jpCldCmd)["value"].as<int>();

				//char buf[64];
				//sprintf(buf, "%d;%d;%d;%d;%d;%d", node_id, S_DIMMER, C_SET, 1, V_DIMMER, value);
				//String strCmd(buf);
				//ExecuteLightCommand(strCmd);
				// Use shortcut instead
				String strCmd = String::format("%d:%d:%d", node_id, (_cmd == CMD_BRIGHTNESS ? 9 : 11), value);
				return theRadio.ProcessSend(strCmd, 0, sub_id);
			}
		}
  }*/

  if( rc == 0 ) {
		LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
		return 0;
	}
	return 1;
}

int SmartControllerClass::ExeJSONConfig(String jsonData)
{ // TODO
  return 1;
}

// Process all kinds of commands
void SmartControllerClass::ProcessCommands()
{
	// Process Local Bridge Commands
	ProcessLocalCommands();

	// Process Cloud Commands
	ProcessCloudCommands();
}

// Process Local bridge commands
void SmartControllerClass::ProcessLocalCommands() {
	theRadio.PeekMessage();
	theRadio.ProcessMQ();

	// Process Console Command
	SERIAL_LN("processCommand...");
  theConsole.processCommand();
  SERIAL_LN("processCommand end");
}

// Process Cloud Commands
void SmartControllerClass::ProcessCloudCommands()
{
	String _cmd;
	while( m_cmdList.size() ) {
		_cmd = m_cmdList.shift();
		ExeJSONCommand(_cmd);
	}
	while( m_configList.size() ) {
		_cmd = m_configList.shift();
		ExeJSONConfig(_cmd);
	}
}

BOOL SmartControllerClass::SelfCheck(US ms)
{
	static US tickSaveConfig = 0;				// must be static
	static US tickCheckRadio = 0;				// must be static
	static UC tickAcitveCheck = 0;
	static UC tickWiFiOff = 0;

	// Save config if it was changed
	if (++tickSaveConfig > 30000 / ms) {	// once per 30 seconds
		tickSaveConfig = 0;
		theConfig.SaveConfig();
	}
  // TODO
	// Scan device list and check keepalive timeout
	/*if( tickSaveConfig % (2000 / ms) == 0 ) { // every 2 second
		CheckDevTimeout();
	}*/

	// TODO Publish relay key status if changed
	/*if( !theConfig.GetDisableWiFi() ) {
		if( Particle.connected() ) PublishRelayKeyFlag();
	}*/

  // Slow Checking: once per 60 seconds
  if (++tickCheckRadio > 60000 / ms) {
		// Check RF module
		++tickAcitveCheck;
		tickCheckRadio = 0;
    //TODO rf check
    /*if( !IsRFGood() || !theRadio.CheckConfig() ) {
      if( CheckRF() ) {
				theRadio.switch2BaseNetwork();
				delay(10);
				theRadio.switch2MyNetwork();
        LOGN(LOGTAG_MSG, "RF24 moudle recovered.");
      }
    } else {
			// Try to send testing message
			String strCmd = String::format("255:8");
			theRadio.ProcessSend(strCmd);
		}*/

		// Check Network
		if( !theConfig.GetDisableWiFi() ) {
			if( theConfig.GetWiFiStatus() ) {
				if( !IsWANGood() || tickAcitveCheck % 5 == 0 || GetStatus() == STATUS_DIS ) {
					InitNetwork();
				}

				if( IsWANGood() ) { // WLAN is good
					SERIAL_LN("Wan is good...");
					tickWiFiOff = 0;
					if( !Particle.connected() ) {
						// Cloud disconnected, try to recover
						if( theConfig.GetUseCloud() != CLOUD_DISABLE ) {
							connectCloud();
						}
					} else {
						if( theConfig.GetUseCloud() == CLOUD_DISABLE ) {
							Particle.disconnect();
						}
					}
				} else { // WLAN is wrong
					Serial.printlnf("Wan is wrong...tickwifioff=%d",tickWiFiOff);
					if( ++tickWiFiOff > 5 ) {
						theConfig.SetWiFiStatus(false);
						if( theConfig.GetUseCloud() == CLOUD_MUST_CONNECT ) {
							SERIAL_LN("System is about to reset due to lost of network...");
							Restart();
						} else {
							// Avoid keeping trying
							LOGE(LOGTAG_MSG, "Turn off WiFi!");
							WiFi.disconnect();
							WiFi.off();	// In order to resume Wi-Fi, restart the application
						}
					}
				}
			} else if( WiFi.ready() ) {
				theConfig.SetWiFiStatus(true);
			}

			// Daily Cloud Synchronization
			/// TimeSync
			theConfig.CloudTimeSync(false);
		}
  }

	// Check System Status
	if( GetStatus() == STATUS_ERR ) {
		LOGE(LOGTAG_MSG, "System is about to reset due to STATUS_ERR...");
		Restart();
	}

	// ToDo:add any other potential problems to check
	//...

	return true;
}

int SmartControllerClass::CldSetTimeZone(String tzStr)
{
}
int SmartControllerClass::CldPowerSwitch(String swStr)
{}
int SmartControllerClass::CldSetCurrentTime(String tmStr)
{}
void SmartControllerClass::OnSensorDataChanged(const UC _sr, const UC _nd)
{}
