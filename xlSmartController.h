//  xlSmartController.h - Xlight SmartController project scale definitions header

#ifndef xlSmartController_h
#define xlSmartController_h

#include "xlxCloudObj.h"
//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
class SmartControllerClass : public CloudObjClass
{
private:
  BOOL m_isLAN;
  BOOL m_isWAN;
  BOOL m_isRF;
public:
  SmartControllerClass();
  /*void Init();
  void InitNetwork();
  void InitPins();
  //void InitCloudObj();

  UC GetStatus();
  BOOL SetStatus(UC st);*/

  BOOL IsLANGood();
  BOOL IsWANGood();
  BOOL IsRFGood();

  /*BOOL CheckWiFi();
  BOOL CheckNetwork();*/


  int CldSetTimeZone(String tzStr);
  int CldPowerSwitch(String swStr);
  int CldSetCurrentTime(String tmStr);
  void OnSensorDataChanged(const UC _sr, const UC _nd);
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern SmartControllerClass theSys;

#endif /* xlSmartController_h */
