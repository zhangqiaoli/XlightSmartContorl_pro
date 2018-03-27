// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Long Click
//
// (c) 2010 karl@pitrich.com
// (c) 2014 karl@pitrich.com
//
// Timer-based rotary encoder logic by Peter Dannegger
// http://www.mikrocontroller.net/articles/Drehgeber
// ----------------------------------------------------------------------------

#ifndef __have__ClickEncoder_h__
#define __have__ClickEncoder_h__

// ----------------------------------------------------------------------------

#include "application.h"

// ----------------------------------------------------------------------------

#define ENC_NORMAL        (1 << 1)   // use Peter Danneger's decoder
#define ENC_FLAKY         (1 << 2)   // use Table-based decoder

// ----------------------------------------------------------------------------

#ifndef ENC_DECODER
#define ENC_DECODER     ENC_NORMAL   //ENC_FLAKY, ENC_NORMAL
#endif

#if ENC_DECODER == ENC_FLAKY
#ifndef ENC_HALFSTEP
#define ENC_HALFSTEP    1        // use table for half step per default
#endif
#endif

// ----------------------------------------------------------------------------

enum ButtonType {
    BUTTON_OPEN = 0,
    BUTTON_CLOSED,

    BUTTON_PRESSED,
    BUTTON_HELD,
    BUTTON_RELEASED,

    BUTTON_CLICKED,
    BUTTON_DOUBLE_CLICKED

};

class ClickEncoder
{
private:
  const uint8_t _pinA;
  const uint8_t _pinB;
  const uint8_t _pinBTN;
  const bool _pinsActive;
  volatile int16_t _delta;
  volatile int16_t _last;
  volatile int8_t _direction;
  uint8_t _steps;
  volatile uint16_t _acceleration;
  uint32_t _heldStartTick;
  volatile uint8_t _heldDuration;

#if ENC_DECODER != ENC_NORMAL
  static const int8_t _table[16];
#endif

#ifndef WITHOUT_BUTTON
  volatile ButtonType _button;
  bool _doubleClickEnabled;
  bool _accelerationEnabled;
#endif

public:
  ClickEncoder(uint8_t A, uint8_t B, uint8_t buttonPin = -1,
               uint8_t stepsPerNotch = 1, bool active = LOW);

  void service(void);
  int16_t getValue(void);

#ifndef WITHOUT_BUTTON
  ButtonType getButton(void);
#endif

#ifndef WITHOUT_BUTTON
  void setDoubleClickEnabled(const bool &d)
  {
    _doubleClickEnabled = d;
  }

  const bool getDoubleClickEnabled()
  {
    return _doubleClickEnabled;
  }

  const uint8_t getHeldDuration()
  {
    return _heldDuration;
  }

#endif

  void setAccelerationEnabled(const bool &a)
  {
    _accelerationEnabled = a;
    if (_accelerationEnabled == false) {
      _acceleration = 0;
    }
  }

  const bool getAccelerationEnabled()
  {
    return _accelerationEnabled;
  }

  const int8_t getDirection()
  {
    return _direction;
  }
};

// ----------------------------------------------------------------------------

#endif // __have__ClickEncoder_h__
