//  xliCloudObj.h - Xlight cloud variables and functions definitions header

#ifndef xliCloudObj_h
#define xliCloudObj_h

#include "xliCommon.h"
#include "ArduinoJson.h"
#include "LinkedList.h"
#include "MoveAverage.h"

// Comment it off if we don't use Particle public cloud
/// Notes:
/// Currently, Particle public cloud supports up to 20 variables and 15 functions.

// Notes: Variable name max length is 12 characters long.
// Cloud variables
#define CLV_SysID               "sysID"           // Can also be a Particle Object
#define CLV_AppVersion          "appVersion"      // Can also be a Particle Object
#define CLV_TimeZone            "timeZone"        // Can also be a Particle Object
#define CLV_SysStatus           "sysStatus"       // Can also be a Particle Object
#define CLV_LastMessage         "lastMsg"         // Can also be a Particle Object

// Notes: The length of the funcKey is limited to a max of 12 characters.
// Cloud functions
#define CLF_SetTimeZone         "SetTimeZone"     // Can also be a Particle Object
#define CLF_PowerSwitch         "PowerSwitch"     // Can also be a Particle Object
#define CLF_JSONCommand         "JSONCommand"     // Can also be a Particle Object
#define CLF_JSONConfig          "JSONConfig"      // Can also be a Particle Object
#define CLF_SetCurTime          "SetCurTime"

typedef struct
{
  UC node_id;                       // RF nodeID
  float data;
} nd_float_t;

typedef struct
{
  UC node_id;                       // RF nodeID
  US data;
} nd_us_t;

typedef struct
{
  UC node_id;                       // RF nodeID
  UC data;
} nd_uc_t;

//------------------------------------------------------------------
// Xlight CloudObj Class
//------------------------------------------------------------------
class CloudObjClass
{
public:
  // Variables should be defined as public
  String m_SysID;
  String m_SysVersion;    // firmware version
  int m_nAppVersion;
  int m_SysStatus;
  String m_tzString;
  String m_lastMsg;
  String m_strCldCmd;

  // Sensor Data from Controller
  CMoveAverage m_sysTemp;
  CMoveAverage m_sysHumi;

  // Sensor Data from Node
  nd_float_t m_temperature;
  nd_float_t m_humidity;
  nd_us_t m_brightness;
  nd_uc_t m_motion;
  nd_uc_t m_irKey;
  nd_us_t m_gas;
  nd_us_t m_smoke;
  nd_uc_t m_sound;
  nd_us_t m_noise;
  nd_us_t m_pm25;
  nd_us_t m_pm10;
  nd_float_t m_tvoc;
  nd_float_t m_ch2o;
  nd_us_t m_co2;

public:
  CloudObjClass();

  String GetSysID();
  String GetSysVersion();

  int CldJSONCommand(String jsonCmd);
  int CldJSONConfig(String jsonData);
  virtual int CldSetTimeZone(String tzStr) = 0;
  virtual int CldPowerSwitch(String swStr) = 0;
  virtual int CldSetCurrentTime(String tmStr) = 0;
  virtual void OnSensorDataChanged(const UC _sr, const UC _nd) = 0;
  int ProcessJSONString(const String& inStr);

  BOOL UpdateDHT(uint8_t nid, float _temp, float _humi);
  BOOL UpdateBrightness(uint8_t nid, uint8_t value);
  BOOL UpdateMotion(uint8_t nid, uint8_t sensor, uint8_t value);
  BOOL UpdateGas(uint8_t nid, uint16_t value);
  BOOL UpdateDust(uint8_t nid, uint16_t value);
  BOOL UpdateSmoke(uint8_t nid, uint16_t value);
  BOOL UpdateSound(uint8_t nid, uint8_t value);
  BOOL UpdateNoise(uint8_t nid, uint16_t value);
  BOOL UpdateAirQuality(uint8_t nid, uint16_t pm25,uint16_t pm10,float tvoc,float ch2o,uint16_t co2);

  BOOL PublishMsg(uint8_t msgType,const char *msg,uint8_t len,uint8_t nid=0xff,UC bNeedReplace=0);
  //BOOL PublishDeviceStatus(const char *msg,uint8_t len);
  //BOOL PublishDeviceConfig(const char *msg,uint8_t len);
  //BOOL PublishACDeviceStatus(const char *msg,uint8_t len);

  void GotNodeConfigAck(const UC _nodeID, const UC *data);
  //BOOL PublishAlarm(const char *msg);

protected:
  void InitCloudObj();

  JsonObject *m_jpCldCmd;

  LinkedList<String> m_cmdList;
  LinkedList<String> m_configList;
};

#endif /* xliCloudObj_h */
