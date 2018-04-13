//  xlxPanel.h - Xlight SmartController Panel Class
#ifndef xlxPanel_h
#define xlxPanel_h

#include "application.h"

class xlPanelClass
{
private:
  int16_t m_nDimmerValue;
  int16_t m_nCCTValue;
  bool m_stSwitch;
  uint32_t m_nLastOpPast;

public:
  xlPanelClass();
  virtual ~xlPanelClass();

  bool GetOnOff();
  void SetOnOff(bool _switch);

  int16_t GetDimmerValue();
  void SetDimmerValue(int16_t _value);
  void UpdateDimmerValue(int16_t _value);
  int16_t GetCCTValue();
  void SetCCTValue(int16_t _value);
  void UpdateCCTValue(uint16_t _value);

};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern xlPanelClass thePanel;

#endif /* xlxPanel_h */
