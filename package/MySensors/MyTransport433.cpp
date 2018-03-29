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

#include "MyTransport433.h"

MyTransport433::MyTransport433(uint8_t address,uint8_t channel)
	:
	_address(address),_channel(channel)
{
	_bValid = false;
}

bool MyTransport433::init() {
	// Start up the radio library
	if (!cc1101433.begin()) {
		_bValid = false;
		return false;
	}
	cc1101433.set_channel(_channel);
	cc1101433.set_myaddr(_address);
	
	cc1101433.show_register_settings();
	_bValid = true;
	return true;
}

bool MyTransport433::init(uint8_t channel,uint8_t address)
{
	_channel = channel;
	_address = address;
	return init();
}

void MyTransport433::setAddress(uint8_t address) {
  if(_address != address)
	{
  	_address = address;
		cc1101433.set_myaddr(address);
	}
}

uint8_t MyTransport433::getAddress() {
	return _address;
}

bool MyTransport433::isValid() {
	return _bValid;
}

bool MyTransport433::CheckConfig()
{
	if( cc1101433.get_channel() != _channel ) return false;
	return true;
}

bool MyTransport433::send(uint8_t to, const void* data, uint8_t len) {
	uint8_t sndmsg[FIFOBUFFER];
	memset(sndmsg,0x00,FIFOBUFFER);
	sndmsg[0]=len+2;
	sndmsg[1]=to;
	sndmsg[2]=_address;
	memcpy(sndmsg+3,data,len);
	uint8_t ok = cc1101433.sent_packet(_address,to,sndmsg,len+3,5);
	if(ok) return true;
	return false;
}

bool MyTransport433::send(uint8_t to, MyMessage &message) {
	message.setVersion(PROTOCOL_VERSION);
	message.setLast(_address);
	uint8_t length = message.getSigned() ? MAX_MESSAGE_LENGTH : message.getLength();
	return send(to, (void *)&(message.msg), min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length));
}

bool MyTransport433::available() {
	if(cc1101433.packet_available() == TRUE)
	{
		return true;
	}
	return false;
}

uint8_t MyTransport433::receive(void* data,uint8_t *from,uint8_t *to) {
	uint8_t Rx_fifo[FIFOBUFFER];
  uint8_t pktlen, lqi,rx_addr,sender;
	int8_t rssi_dbm;
	uint8_t datalen = 0;
	if(cc1101433.packet_available() == TRUE)
	{
		if(cc1101433.get_payload(Rx_fifo, pktlen, rx_addr, sender, rssi_dbm, lqi) == TRUE) //stores the payload data to Rx_fifo
		 {
			   *to = rx_addr;
				 *from = sender;
				 datalen = pktlen - 3;
				 memcpy(data,(uint8_t *) Rx_fifo + 3,datalen);
		 }
	}
	return datalen;
}

void MyTransport433::powerDown() {
	cc1101433.powerdown();
}

uint8_t MyTransport433::getChannel(bool read)
{
	if( read ) {
		_channel = cc1101433.get_channel();
	}
	return _channel;
}

void MyTransport433::setChannel(uint8_t channel)
{
	if( _channel != channel )	{
		_channel = channel;
		cc1101433.set_channel(channel);
	}
}
