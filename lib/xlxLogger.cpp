/**
 * xlxLogger.cpp - Xlight logger library for application data logging
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
 * 1. Define basic interfaces
 * 2. Serial logging
 *
 * ToDo:
 * 1. syslog, refer to psyslog.cpp
 * 2. http/cloud
 * 3. Flash
 * 4. Offline Data Cache in Flash (loop overwrite)
**/

#include "xlxLogger.h"
#include "xlSmartController.h"

// the one and only instance of LoggerClass
LoggerClass theLog = LoggerClass();
char strDestNames[][7] = {"serial", "flash", "syslog", "cloud", "all"};
char strLevelNames[][9] = {"none", "alert", "critical", "error", "warn", "notice", "info", "debug"};

//------------------------------------------------------------------
// Xlight Logger Class
//------------------------------------------------------------------
LoggerClass::LoggerClass()
{
  m_level[LOGDEST_SERIAL] = LEVEL_DEBUG;
  m_level[LOGDEST_FLASH] = LEVEL_WARNING;
  m_level[LOGDEST_SYSLOG] = LEVEL_INFO;
  m_level[LOGDEST_CLOUD] = LEVEL_NOTICE;
}

void LoggerClass::Init(String sysid)
{
  m_SysID = sysid;
}

BOOL LoggerClass::InitFlash(UL addr, UL size)
{
  //ToDo:
  return true;
}

BOOL LoggerClass::InitSysLog(String host, US port)
{
  //ToDo:
  return true;
}

BOOL LoggerClass::InitCloud(String url, String uid, String key)
{
  //ToDo:
  return true;
}

UC LoggerClass::GetLevel(UC logDest)
{
  if( logDest < LOGDEST_DUMMY)
    return m_level[logDest];

  return LEVEL_EMERGENCY;
}

void LoggerClass::SetLevel(UC logDest, UC logLevel)
{
  if( logDest < LOGDEST_DUMMY)
  {
    if( m_level[logDest] != logLevel )
      m_level[logDest] = logLevel;
  }
}

void LoggerClass::WriteLog(UC level, const char *tag, const char *msg, ...)
{
  int nSize = 0;
  char buf[MAX_MESSAGE_LEN];

  // Prepare message
  int nPos = snprintf(buf, MAX_MESSAGE_LEN, "%02d:%02d:%02d %d %s ",
      Time.hour(), Time.minute(), Time.second(), level, tag);
  va_list args;
  va_start(args, msg);
  nSize = vsnprintf(buf + nPos, MAX_MESSAGE_LEN - nPos, msg, args);

  // Send message to serial port
  if( level <= m_level[LOGDEST_SERIAL] )
  {
    TheSerial.println(buf);
  }

  // Output Log to Particle cloud variable
  if( level <= m_level[LOGDEST_CLOUD] ) {
    theSys.PublishLog(buf);
  }

  // ToDo: send log to other destinations
  //if( level <= m_level[LOGDEST_SYSLOG] ) {
  //;}
  //if( level <= m_level[LOGDEST_FLASH] ) {
  //}
}

bool LoggerClass::ChangeLogLevel(String &strMsg)
{
	int nPos = strMsg.indexOf(':');
  UC lv_Dest, lv_Level;
  String lv_sDest, lv_sLevel;
  if( nPos > 0 ) {
    lv_sDest = strMsg.substring(0, nPos);
    lv_sLevel = strMsg.substring(nPos+1);
  } else {
    lv_sDest = strDestNames[LOGDEST_DUMMY];
    lv_sLevel = strMsg;
  }

  // Parse Log Level
  for( lv_Level = LEVEL_EMERGENCY; lv_Level <= LEVEL_DEBUG; lv_Level++ ) {
    if( lv_sLevel.equals(strLevelNames[lv_Level]) ) break;
  }
  if( lv_Level > LEVEL_DEBUG ) return false;

  // Parse Log Destination
  for( lv_Dest = LOGDEST_SERIAL; lv_Dest <= LOGDEST_DUMMY; lv_Dest++ ) {
    if( lv_sDest.equals(strDestNames[lv_Dest]) ) break;
  }
  if( lv_Dest > LOGDEST_DUMMY ) return false;

  if( lv_Dest  == LOGDEST_DUMMY ) {
    for( lv_Dest = LOGDEST_SERIAL; lv_Dest < LOGDEST_DUMMY; lv_Dest++ ) {
      SetLevel(lv_Dest, lv_Level);
    }
  } else {
    SetLevel(lv_Dest, lv_Level);
  }

  return true;
}

String LoggerClass::PrintDestInfo()
{
  String strShortDesc = "";

  UC lv_Dest;
  for( lv_Dest = LOGDEST_SERIAL; lv_Dest < LOGDEST_DUMMY; lv_Dest++ ) {
    SERIAL_LN("LOG Channel: %s - level: %s", strDestNames[lv_Dest], strLevelNames[GetLevel(lv_Dest)]);
    if( lv_Dest != LOGDEST_SERIAL ) {
      strShortDesc += "; ";
    }
    strShortDesc += strLevelNames[GetLevel(lv_Dest)];
    strShortDesc += "@";
    strShortDesc += strDestNames[lv_Dest];
  }
  SERIAL_LN("");

  return strShortDesc;
}
