/**
 * xlxConfig.cpp - Xlight Config library for loading & saving configuration
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
 * 1. configurations are stored in the emulated EEPROM and P1 external Flash
 * 1.1 The Photon and Electron have 2047 bytes of emulated EEPROM
 *     with address space from 0x0000 to 0x07FF.
 * 1.2 The P1 has 1M bytes of external Flash, but can only be accessed via
 *     3rd party API currently.
 * 2. Use EEPROM class (high level API) to access the emulated EEPROM.
 * 3. Use spark-flashee-eeprom (low level 3rd party API) to access P1 external Flash.
 * 4. Please refer to xliMemoryMap.h for memory allocation.
 *
 * ToDo:
 * 1. Move default config values to header as global #define's
 * 2.
 * 3.
**/

#include "xlxConfig.h"
#include "xliPinMap.h"
#include "xlxLogger.h"
#include "xliMemoryMap.h"
#include "xliNodeConfig.h"
#include "xlxPanel.h"
#include "xlxRF433Server.h"
#include "xlSmartController.h"

using namespace Flashee;

// the one and only instance of ConfigClass
ConfigClass theConfig = ConfigClass();

//------------------------------------------------------------------
// Xlight Node List Class
//------------------------------------------------------------------
int NodeListClass::getMemSize()
{
	return(sizeof(NodeIdRow_t) * _count);
}

int NodeListClass::getFlashSize()
{
	return(min(sizeof(NodeIdRow_t) * _size, MEM_NODELIST_LEN));
}

//------------------------------------------------------------------
// Xlight Config Class
//------------------------------------------------------------------
ConfigClass::ConfigClass()
{
	P1Flash = Devices::createWearLevelErase();

  m_isLoaded = false;
  m_isChanged = false;
	m_lastTimeSync = millis();
  InitConfig();
}

void ConfigClass::InitConfig()
{
  memset(&m_config, 0x00, sizeof(m_config));
  m_config.version = VERSION_CONFIG_DATA;
  m_config.timeZone.id = 90;              // Toronto
  m_config.timeZone.offset = -300;        // -5 hours
  m_config.timeZone.dst = 1;              // 1 or 0
  strcpy(m_config.Organization, XLA_ORGANIZATION);
  strcpy(m_config.ProductName, XLA_PRODUCT_NAME);
}

BOOL ConfigClass::InitDevStatus(UC nodeID)
{
  return false;
}

BOOL ConfigClass::GetDisableWiFi()
{
  return m_config.disableWiFi;
}

BOOL ConfigClass::SetDisableWiFi(BOOL _st)
{
  if( _st != m_config.disableWiFi ) {
    m_config.disableWiFi = _st;
    m_isChanged = true;
		SERIAL_LN("Wi-Fi chip %s", _st ? "disabled" : "enabled");
    return true;
  }
  return false;
}

BOOL ConfigClass::GetDisableLamp()
{
	return m_config.disableLamp;
}

BOOL ConfigClass::SetDisableLamp(BOOL _st)
{
	if( _st != m_config.disableLamp ) {
		m_config.disableLamp = _st;
		m_isChanged = true;
		SERIAL_LN("Lamp chip %s", _st ? "disabled" : "enabled");
		return true;
	}
	return false;
}

UC ConfigClass::GetUseCloud()
{
	return m_config.useCloud;
}

BOOL ConfigClass::SetUseCloud(UC opt)
{
	if( opt != m_config.useCloud && opt <= CLOUD_MUST_CONNECT ) {
		m_config.useCloud = opt;
		m_isChanged = true;
		return true;
	}
	return false;
}

UC ConfigClass::GetBrightIndicator()
{
  return m_config.indBrightness;
}

BOOL ConfigClass::SetBrightIndicator(UC level)
{
  if( level != m_config.indBrightness )
  {
    m_config.indBrightness = level;
    m_isChanged = true;
    return true;
  }

  return false;
}

UC ConfigClass::GetRelayKeyObj()
{
	return m_config.hwsObj;
}

BOOL ConfigClass::SetRelayKeyObj(UC _value)
{
	if( _value != m_config.hwsObj ) {
    m_config.hwsObj = _value;
    m_isChanged = true;
    return true;
  }
  return false;
}

UC ConfigClass::GetRelayKeys()
{
	return(m_config.relay_key_value);
}

BOOL ConfigClass::SetRelayKeys(const UC _keys)
{
	if( m_config.relay_key_value != _keys ) {
		m_config.relay_key_value = _keys;
		m_isChanged = true;
		return true;
	}
	return false;
}

UC ConfigClass::GetRelayKey(const UC _code)
{
	return(BITTEST(m_config.relay_key_value, _code));
}

BOOL ConfigClass::SetRelayKey(const UC _code, const UC _on)
{
	UC newValue;
	if( _on ) newValue = BITSET(m_config.relay_key_value, _code);
	else newValue = BITUNSET(m_config.relay_key_value, _code);

	if( newValue != m_config.relay_key_value ) {
		m_config.relay_key_value = newValue;
		m_isChanged = true;
		return true;
	}
	return false;
}

BOOL ConfigClass::GetWiFiStatus()
{
  return m_config.stWiFi;
}

BOOL ConfigClass::SetWiFiStatus(BOOL _st)
{
  if( _st != m_config.stWiFi ) {
    m_config.stWiFi = _st;
    m_isChanged = true;
		LOGN(LOGTAG_STATUS, "Wi-Fi status set to %d", _st);
    return true;
  }
  return false;
}

BOOL ConfigClass::IsCloudSerialEnabled()
{
	return m_config.enableCloudSerialCmd;
}

void ConfigClass::SetCloudSerialEnabled(BOOL sw)
{
	if( sw != m_config.enableCloudSerialCmd ) {
		m_config.enableCloudSerialCmd = sw;
		m_isChanged = true;
	}
}

UC ConfigClass::GetRFChannel()
{
	return m_config.rfChannel;
}

BOOL ConfigClass::SetRFChannel(UC channel)
{
	if( channel > 127 ) channel = CC1101_433_CHANNEL;
	if( channel != m_config.rfChannel ) {
		m_config.rfChannel = channel;
		theRadio.setChannel(channel);
		m_isChanged = true;
		return true;
	}
	return false;
}

UC ConfigClass::GetRFAddr()
{
	return m_config.rfAddr;
}

BOOL ConfigClass::SetRFAddr(UC addr)
{
	if( addr > 127 ) addr = CC1101_433_ADDRESS;
	if( addr != m_config.rfAddr ) {
		m_config.rfAddr = addr;
		theRadio.setAddress(addr);
		m_isChanged = true;
		return true;
	}
	return false;
}

BOOL ConfigClass::MemWriteScenarioRow(ScenarioRow_t row, uint32_t address)
{
#ifdef MCU_TYPE_P1
	return P1Flash->write<ScenarioRow_t>(row, address);
#else
	return false;
#endif
}

BOOL ConfigClass::MemReadScenarioRow(ScenarioRow_t &row, uint32_t address)
{
#ifdef MCU_TYPE_P1
	return P1Flash->read<ScenarioRow_t>(row, address);
#else
	return false;
#endif
}

BOOL ConfigClass::IsValidConfig()
{
	return true;
}

BOOL ConfigClass::LoadConfig()
{
  // Load System Configuration
  if( sizeof(Config_t) <= MEM_CONFIG_LEN )
  {
    EEPROM.get(MEM_CONFIG_OFFSET, m_config);
    if(!IsValidConfig())
    {
	  LOGW(LOGTAG_MSG, "Sysconfig is empty, load backup config from flash.");
	  LoadBackupConfig();
	  if(!IsValidConfig())
	  {
        InitConfig();
        m_isChanged = true;
        LOGW(LOGTAG_MSG, "Sys backconfig is empty, use default settings.");
        SaveConfig();
	  }
	  else
      {
		LOGW(LOGTAG_MSG, "Sys backconfig loaded.");
      }
    }
    else
    {
			m_config.version = VERSION_CONFIG_DATA;
      LOGW(LOGTAG_MSG, "Sysconfig loaded.");
    }
    m_isLoaded = true;
    m_isChanged = false;
		UpdateTimeZone();
  } else {
    LOGE(LOGTAG_MSG, "Failed to load Sysconfig, too large.");
  }

  return m_isLoaded;
}

BOOL ConfigClass::SaveConfig()
{
	// Check changes on Panel
	//SetBrightIndicator(thePanel.GetDimmerValue());

  if( m_isChanged )
  {
    EEPROM.put(MEM_CONFIG_OFFSET, m_config);
    m_isChanged = false;
    //LOGI(LOGTAG_MSG, "Sysconfig saved.");
	  uint8_t attemps = 0;
	  while(++attemps <=3 )
    {
		  if(SaveBackupConfig())
			{
				LOGN(LOGTAG_MSG, "Sysconfig saved success!");
				break;
			}
			else
			{
        LOGW(LOGTAG_MSG, "Sysconfig saved failed,tried %d",attemps);
			}
    }
  }

  return true;
}

BOOL ConfigClass::IsConfigLoaded()
{
  return m_isLoaded;
}

UC ConfigClass::GetVersion()
{
  return m_config.version;
}

US ConfigClass::GetTimeZoneID()
{
	return m_config.timeZone.id;
}

BOOL ConfigClass::SetTimeZoneID(US tz)
{
	if( tz == 0 || tz > 500 )
		return false;

	if( tz != m_config.timeZone.id )
	{
		m_config.timeZone.id = tz;
		m_isChanged = true;
		theSys.m_tzString = GetTimeZoneJSON();
	}
	return true;
}

void ConfigClass::UpdateTimeZone()
{
	// Change System Timezone
	Time.zone((float)GetTimeZoneOffset() / 60 + GetDaylightSaving());
}

UC ConfigClass::GetDaylightSaving()
{
	return m_config.timeZone.dst;
}

BOOL ConfigClass::SetDaylightSaving(UC flag)
{
	if( flag > 1 )
		return false;

	if( flag != m_config.timeZone.dst )
	{
		m_config.timeZone.dst = flag;
		m_isChanged = true;
		theSys.m_tzString = GetTimeZoneJSON();
		UpdateTimeZone();
	}
	return true;
}

SHORT ConfigClass::GetTimeZoneOffset()
{
	return m_config.timeZone.offset;
}

SHORT ConfigClass::GetTimeZoneDSTOffset()
{
	return (m_config.timeZone.offset + m_config.timeZone.dst * 60);
}

BOOL ConfigClass::SetTimeZoneOffset(SHORT offset)
{
	if( offset >= -780 && offset <= 780)
	{
		if( offset != m_config.timeZone.offset )
		{
			m_config.timeZone.offset = offset;
			m_isChanged = true;
			theSys.m_tzString = GetTimeZoneJSON();
			// Change System Timezone
			UpdateTimeZone();
		}
		return true;
	}
	return false;
}

String ConfigClass::GetTimeZoneJSON()
{
	String jsonStr = String::format("{\"id\":%d,\"offset\":%d,\"dst\":%d}",
									GetTimeZoneID(), GetTimeZoneOffset(), GetDaylightSaving());
	return jsonStr;
}

BOOL ConfigClass::IsDailyTimeSyncEnabled()
{
	return m_config.enableDailyTimeSync;
}

void ConfigClass::SetDailyTimeSyncEnabled(BOOL sw)
{
	if( sw != m_config.enableDailyTimeSync ) {
		m_config.enableDailyTimeSync = sw;
		m_isChanged = true;
	}
}

void ConfigClass::DoTimeSync()
{
	// Request time synchronization from the Particle Cloud
	Particle.syncTime();
	m_lastTimeSync = millis();
	LOGI(LOGTAG_EVENT, "Time synchronized");
}

BOOL ConfigClass::CloudTimeSync(BOOL _force)
{
	if( Particle.connected() ) {
		if( _force ) {
			DoTimeSync();
			return true;
		} else if( theConfig.IsDailyTimeSyncEnabled() ) {
			if( (millis() - m_lastTimeSync) / 1000 > SECS_PER_DAY ) {
				DoTimeSync();
				return true;
			}
		}
	}
	return false;
}


String ConfigClass::GetOrganization()
{
	String strName = m_config.Organization;
	return strName;
}

void ConfigClass::SetOrganization(const char *strName)
{
	strncpy(m_config.Organization, strName, sizeof(m_config.Organization) - 1);
	m_isChanged = true;
}

String ConfigClass::GetProductName()
{
	String strName = m_config.ProductName;
	return strName;
}

void ConfigClass::SetProductName(const char *strName)
{
	strncpy(m_config.ProductName, strName, sizeof(m_config.ProductName) - 1);
	m_isChanged = true;
}



BOOL ConfigClass::LoadBackupConfig()
{
#ifdef MCU_TYPE_P1
	return P1Flash->read<Config_t>(m_config, MEM_CONFIG_BACKUP_OFFSET);
#else
	return false;
#endif
}


BOOL ConfigClass::SaveBackupConfig()
{
#ifdef MCU_TYPE_P1
	return P1Flash->write<Config_t>(m_config, MEM_CONFIG_BACKUP_OFFSET);
#else
	return false;
#endif
}
