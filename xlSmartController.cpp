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
	//return (m_isRF && theRadio.isValid());
  return true;
}

/*UC SmartControllerClass::GetStatus()
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
}*/

/*// Primitive initialization before loading configuration
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
}*/

int SmartControllerClass::CldSetTimeZone(String tzStr)
{

}
int SmartControllerClass::CldPowerSwitch(String swStr)
{}
int SmartControllerClass::CldSetCurrentTime(String tmStr)
{}
void SmartControllerClass::OnSensorDataChanged(const UC _sr, const UC _nd)
{}
