/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef MyTransport433_h
#define MyTransport433_h

#include "application.h"

#include "MyConfig.h"
#include "MyMessage.h"
#include "MyTransport.h"
#include "cc1100.h"

class MyTransport433 : public MyTransport
{
public:
	MyTransport433(uint8_t address=CC1101_433_ADDRESS,uint8_t channel=CC1101_433_CHANNEL);
	bool init();
	bool init(uint8_t channel,uint8_t address);
	void setAddress(uint8_t address);
	uint8_t getAddress();
	bool send(uint8_t to, const void* data,uint8_t len);
	bool send(uint8_t to, MyMessage &message);
	bool available();
	uint8_t receive(void* data,uint8_t *from,uint8_t *to);
	void powerDown();

	uint8_t getChannel(bool read = true);
	void setChannel(uint8_t channel);

	bool isValid();
	bool CheckConfig();
	void PrintRFDetails();

private:
	CC1100 cc1101433;
	uint8_t _address;
	uint8_t _channel;
  bool _bValid;
};

#endif
