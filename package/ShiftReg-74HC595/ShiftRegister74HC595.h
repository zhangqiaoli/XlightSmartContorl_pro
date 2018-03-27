/*
  ShiftRegister74HC595.h - Library for easy control of the 74HC595 shift register.
  Created by Timo Denk (www.simsso.de), Nov 2014.
  Additional information are available on http://shiftregister.simsso.de/
  Released into the public domain.
*/

#ifndef ShiftRegister74HC595_h
#define ShiftRegister74HC595_h

#include "application.h"

class ShiftRegister74HC595
{
public:
    ShiftRegister74HC595(uint8_t numberOfShiftRegisters, uint8_t serialDataPin, uint8_t clockPin, uint8_t latchPin);
    virtual ~ShiftRegister74HC595();
    void setAll(uint8_t * digitalValues);
    uint8_t * getAll();
    void set(uint8_t pin, uint8_t value);
    void setAllLow();
    void setAllHigh();
    uint8_t get(uint8_t pin);
    uint8_t getNumberOfShiftRegisters();

private:
    uint8_t _numberOfShiftRegisters;
    uint8_t _clockPin;
    uint8_t _serialDataPin;
    uint8_t _latchPin;
    uint8_t * _digitalValues;
};

#endif
