/**
 * xlxPanel.cpp - Xlight SmartController Panel Class
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
 * FEATURE DESCRIPTION
 * 1. Click-rotation knob
 * 2. LED brightness indicator
 * 3. RGB status LED
 * 4. Click-knob actions:
 * 4.1 BUTTON_CLICKED: switch the lights on/off
 * 4.2 BUTTON_DOUBLE_CLICKED: switch between BCM (Brightness Control Mode) and CCT control mode temporarily,
 *      if there is no knob spin operation in CCT mode for a few seconds, it will resume to BCM automatically
 * 4.3 BUTTON_HELD / BUTTON_RELEASED, system operations
 *      if >= 30 seconds, then enter DFU mode
 *      if >= 15 seconds, the enter Wi-Fi setup mode
 *      if >= 10 seconds, then reset the controller
 *      if >= 5 seconds, enable base network
 *
**/
#include "xliPinMap.h"
#include "xliNodeConfig.h"
#include "xlxPanel.h"
#include "xlxLogger.h"
#include "xlSmartController.h"
#include "xlxRF433Server.h"
#include "xlxConfig.h"

#define PANEL_NUM_LEDS_RING   12
#define PANEL_NUM_LEDS_CCTIDX 8

// the one and only instance of Panel class
xlPanelClass thePanel;

uint8_t CCWTipLight595 [] = {0x00,0x00,0x01,0x00,0x03,0x00,0x07,0x00,0x0f,0x00,0x1f,0x00,0x3f,0x00,0x7f,0x00,0xff,0x00,0xff,0x01,0xff,0x03,0xff,0x07,0xff,0x0f};
uint8_t CWTipLight595 [] = {0x00,0x00,0x00,0x08,0x00,0x0c,0x00,0x0e,0x00,0x0f,0x80,0x0f,0xc0,0x0f,0xe0,0x0f,0xf0,0x0f,0xf8,0x0f,0xfc,0x0f,0xfe,0x0f,0xff,0x0f};
// LED#: 9, 10, 11, 12, 1, 2, 3, 4, 5
uint8_t CCTIdxLight595 [] = {0x00,0x01, 0x00,0x02, 0x00,0x04, 0x00,0x08, 0x01,0x00, 0x02,0x00, 0x04,0x00, 0x08,0x00, 0x10,0x00};

xlPanelClass::xlPanelClass()
{
  m_nDimmerValue = 0;
  m_nCCTValue = 0;
  m_stSwitch = false;
  m_nLastOpPast = millis();
}

xlPanelClass::~xlPanelClass()
{
}

int16_t xlPanelClass::GetDimmerValue()
{
	return m_nDimmerValue;
}

void xlPanelClass::SetDimmerValue(int16_t _value)
{
  if( m_nDimmerValue != _value ) {
		m_nDimmerValue = _value;
		m_nLastOpPast = millis();
    // Send Light Percentage message
    //theSys.ChangeLampBrightness(CURRENT_DEVICE, _value, CURRENT_SUBDEVICE);
		LOGD(LOGTAG_EVENT, "Dimmer-BR changed to %d", _value);
	}
}

// Update Dimmer value according to confirmation
void xlPanelClass::UpdateDimmerValue(int16_t _value)
{
  uint32_t now = millis();
  if(now - m_nLastOpPast > 2000)
  {
	  // Restrict range
	  m_nDimmerValue = _value;
	  LOGD(LOGTAG_EVENT, "Update BR to %d", _value);
  }
  else
  {
	  LOGD(LOGTAG_EVENT, "ingore update BR to %d", _value);
  }

}

int16_t xlPanelClass::GetCCTValue()
{
	return m_nCCTValue;
}

void xlPanelClass::SetCCTValue(int16_t _value)
{
	if( m_nCCTValue != _value ) {
		m_nCCTValue = _value;
		m_nLastOpPast = millis();
		LOGD(LOGTAG_EVENT, "Dimmer-CCT changed to %d", m_nCCTValue);
	}
}

void xlPanelClass::UpdateCCTValue(uint16_t _value)
{
	uint32_t now = millis();
	if(now - m_nLastOpPast > 2000)
	{
		m_nCCTValue = _value;
		LOGD(LOGTAG_EVENT, "Update cct to %d", m_nCCTValue);
	}
	else
	{
		LOGD(LOGTAG_EVENT, "ingore update cct to %d", m_nCCTValue);
	}
}

bool xlPanelClass::GetOnOff()
{
  return m_stSwitch;
}

void xlPanelClass::SetOnOff(bool _switch)
{
  m_stSwitch = _switch;
}
