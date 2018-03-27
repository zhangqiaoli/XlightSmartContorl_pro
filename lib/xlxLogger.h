//  xlxLogger.h - Xlight Logger library

#ifndef xlxLogger_h
#define xlxLogger_h

#include "xliCommon.h"

#define MAX_MESSAGE_LEN     480

// Log Destination
enum {
    LOGDEST_SERIAL = 0,
    LOGDEST_FLASH,
    LOGDEST_SYSLOG,
    LOGDEST_CLOUD,
    LOGDEST_DUMMY
};

// Log Level
enum {
    LEVEL_EMERGENCY = 0,
    LEVEL_ALERT,
    LEVEL_CRITICAL,
    LEVEL_ERROR,
    LEVEL_WARNING,
    LEVEL_NOTICE,
    LEVEL_INFO,
    LEVEL_DEBUG,
};

// Log tags: 3 bytes
#define LOGTAG_STATUS         "STA"
#define LOGTAG_EVENT          "EVT"
#define LOGTAG_ACTION         "ACT"
#define LOGTAG_DATA           "DAT"
#define LOGTAG_MSG            "MSG"

//------------------------------------------------------------------
// Xlight Logger Class
//------------------------------------------------------------------
class LoggerClass
{
private:
  UC m_level[LOGDEST_DUMMY];
  String m_SysID;

public:
  LoggerClass();
  void Init(String sysid);

  BOOL InitFlash(UL addr, UL size);
  BOOL InitSysLog(String host, US port);
  BOOL InitCloud(String url, String uid, String key);

  UC GetLevel(UC logDest);
  void SetLevel(UC logDest, UC logLevel);
  void WriteLog(UC level, const char *tag, const char *msg, ...);
  bool ChangeLogLevel(String &strMsg);
  String PrintDestInfo();
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern LoggerClass theLog;
#define LOGA(tag, fmt, ...)       theLog.WriteLog(LEVEL_ALERT, tag, fmt, ##__VA_ARGS__)
#define LOGC(tag, fmt, ...)       theLog.WriteLog(LEVEL_CRITICAL, tag, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...)       theLog.WriteLog(LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...)       theLog.WriteLog(LEVEL_WARNING, tag, fmt, ##__VA_ARGS__)
#define LOGN(tag, fmt, ...)       theLog.WriteLog(LEVEL_NOTICE, tag, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...)       theLog.WriteLog(LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...)       theLog.WriteLog(LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)

#endif /* xlxLogger_h */
