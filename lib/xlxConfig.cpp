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
    //theSys.m_tzString = GetTimeZoneJSON();
  }
  return true;
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
