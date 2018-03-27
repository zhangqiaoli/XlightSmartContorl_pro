/**
 * xlxCloudObj.cpp - Xlight Cloud Object library
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
 *
 * ToDo:
 * 1.
**/

#include "MyMessage.h"
#include "xlxCloudObj.h"
#include "xlxConfig.h"
#include "xlxLogger.h"

//------------------------------------------------------------------
// Xlight Cloud Object Class
//------------------------------------------------------------------
CloudObjClass::CloudObjClass()
{
  m_SysID = "";
  m_SysVersion = "";
  m_nAppVersion = VERSION_CONFIG_DATA;
  m_SysStatus = STATUS_OFF;

  m_strCldCmd = "";
}

// Initialize Cloud Variables & Functions
void CloudObjClass::InitCloudObj()
{
    Particle.variable(CLV_SysID, &m_SysID, STRING);
    Particle.variable(CLV_AppVersion, &m_nAppVersion, INT);
    Particle.variable(CLV_TimeZone, &m_tzString, STRING);
    Particle.variable(CLV_SysStatus, &m_SysStatus, INT);
    Particle.variable(CLV_LastMessage, &m_lastMsg, STRING);

    Particle.function(CLF_SetTimeZone, &CloudObjClass::CldSetTimeZone, this);
    Particle.function(CLF_PowerSwitch, &CloudObjClass::CldPowerSwitch, this);
    Particle.function(CLF_JSONCommand, &CloudObjClass::CldJSONCommand, this);
    Particle.function(CLF_JSONConfig, &CloudObjClass::CldJSONConfig, this);
    Particle.function(CLF_SetCurTime, &CloudObjClass::CldSetCurrentTime, this);
}

String CloudObjClass::GetSysID()
{
	return m_SysID;
}

String CloudObjClass::GetSysVersion()
{
	return m_SysVersion;
}

int CloudObjClass::CldJSONCommand(String jsonCmd)
{
  if( m_cmdList.size() > MQ_MAX_CLOUD_MSG ) {
    LOGW(LOGTAG_MSG, "JSON commands exceeded queue size");
    return 0;
  }

  m_cmdList.add(jsonCmd);
  return 1;
}

int CloudObjClass::CldJSONConfig(String jsonData)
{
  if( m_configList.size() > MQ_MAX_CLOUD_MSG ) {
    LOGW(LOGTAG_MSG, "JSON config exceeded queue size");
    return 0;
  }

  m_configList.add(jsonData);
  return 1;
}

// Publish LOG message and update cloud variable
BOOL CloudObjClass::PublishLog(const char *msg)
{
  BOOL rc = true;
  m_lastMsg = msg;
  //if( !theConfig.GetDisableWiFi() ) {
    if( Particle.connected() ) {
      rc = Particle.publish(CLT_NAME_LOGMSG, msg, CLT_TTL_LOGMSG, PRIVATE);
    }
  //}
  return rc;
}

// Concatenate string with regard to the length limitation of cloud API
/// Return value:
/// 0 - string is intact, can be executed
/// 1 - waiting for more input
/// -1 - error
int CloudObjClass::ProcessJSONString(const String& inStr)
{
  String strTemp = inStr;
  StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf;
  m_jpCldCmd = &(lv_jBuf.parseObject(const_cast<char*>(inStr.c_str())));
  /*char* tst = const_cast<char*>(inStr.c_str());
  SERIAL("ptr addr = %p",tst);
  //SERIAL(tst);
  SERIAL(" ,index = ");
  Serial.println(inStr.indexOf("cmd"));*/
  if (!m_jpCldCmd->success())
  {
		if( m_strCldCmd.length() > 0 ) {

			m_strCldCmd.concat(inStr);
			String debugstr = m_strCldCmd;
			StaticJsonBuffer<COMMAND_JSON_SIZE*3 * 8> lv_jBuf2; //buffer size must be larger than COMMAND_JSON_SIZE
			m_jpCldCmd = &(lv_jBuf2.parseObject(const_cast<char*>(m_strCldCmd.c_str())));
			m_strCldCmd = "";		// Already concatenated
			if (!m_jpCldCmd->success()) {
				SERIAL_LN("Could not parse the concatenated string: %s", debugstr.c_str());
				return -1;
			}
		} else {
			return -1;
		}
  }

	if (m_jpCldCmd->containsKey("x0") ) {
		// Begin of a new string
		m_strCldCmd = (*m_jpCldCmd)["x0"];
		return 1;
	}
	else if (m_jpCldCmd->containsKey("x1")) {
		// Concatenate
		strTemp = (*m_jpCldCmd)["x1"];
		m_strCldCmd.concat(strTemp);
		return 1;
  } else if( m_strCldCmd.length() > 0 ) {
		m_strCldCmd.concat(inStr);
		String debugstr = m_strCldCmd;
		StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf3;
		m_jpCldCmd = &(lv_jBuf3.parseObject(const_cast<char*>(m_strCldCmd.c_str())));

		m_strCldCmd = "";		// Already concatenated
		if (!m_jpCldCmd->success()) {
			SERIAL_LN("Could not parse the string: %s", debugstr.c_str());
			return -1;
		}
  }

  return 0;
}
