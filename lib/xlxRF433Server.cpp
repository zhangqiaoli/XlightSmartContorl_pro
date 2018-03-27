/**
 * xlxRF433Server.cpp - Xlight RF2.4 server library based on via RF2.4 module
 * for communication with instances of xlxRF433Client
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
 * Dependancy
 * 1. Particle RF433 library, refer to
 *    https://github.com/stewarthou/Particle-RF433
 *    http://tmrh20.github.io/RF433/annotated.html
 *
 * DESCRIPTION
 * 1. Mysensors protocol compatible and extended
 * 2. Address algorithm
 * 3. PipePool (0 AdminPipe, Read & Write; 1-5 pipe pool, Read)
 * 4. Session manager, optional address shifting
 * 5.
 *
 * ToDo:
 * 1. Two pipes collaboration for security: divide a message into two parts
 * 2. Apply security layer, e.g. AES, encryption, etc.
 *
**/

#include "xlxRF433Server.h"
#include "xliPinMap.h"
#include "xliNodeConfig.h"
#include "xlSmartController.h"
#include "xlxLogger.h"
#include "xlxPanel.h"

#include "MyParserSerial.h"

//------------------------------------------------------------------
// the one and only instance of RF433ServerClass
RF433ServerClass theRadio;
MyMessage msg;
UC *msgData = (UC *)&(msg.msg);

RF433ServerClass::RF433ServerClass()
	:	MyTransport433()
	, CDataQueue(MAX_MESSAGE_LENGTH * MQ_MAX_RF_RCVMSG)
	, CFastMessageQ(MQ_MAX_RF_SNDMSG, MAX_MESSAGE_LENGTH)
{
	_times = 0;
	_succ = 0;
	_received = 0;
}

bool RF433ServerClass::ServerBegin(uint8_t channel,uint8_t address)
{
  // Initialize RF module
	if( !init(channel,address) ) {
    LOGC(LOGTAG_MSG, "RF433 module is not valid!");
		return false;
	}
  return true;
}

bool RF433ServerClass::ProcessMQ()
{
	ProcessSendMQ();
	ProcessReceiveMQ();
	return true;
}

bool RF433ServerClass::ProcessSend(const UC _node, const UC _msgID, String &strPayl, MyMessage &my_msg, const UC _replyTo, const UC _sensor)
{
	return true;
}

bool RF433ServerClass::ProcessSend(String &strMsg, MyMessage &my_msg, const UC _replyTo, const UC _sensor)
{
	int nPos;
	int nPos2;
	uint8_t lv_nNodeID = 0, lv_nSubID;
	uint8_t lv_nMsgID;
	String lv_sPayload = "";

	// Get NodeId & subID
	lv_nSubID = _sensor;
	// send <message> or <NodeID:MessageId[:Payload]>
	// check whether is <message> or <NodeID:MessageId[:Payload]>
	if(strMsg.indexOf(':') > 0)
	{
		//LOGD(LOGTAG_MSG, "nomar msg: %s", strMsg);
		nPos = strMsg.indexOf('-');
		if (nPos > 0) {
			// May contain subID
			lv_nNodeID = (uint8_t)(strMsg.substring(0, nPos).toInt());
			nPos2 = strMsg.indexOf(':', nPos + 1);
			if (nPos2 > 0) {
				lv_nSubID = (uint8_t)(strMsg.substring(nPos + 1, nPos2).toInt());
				nPos = nPos2;
			}
		} else {
			// Has no subID
			nPos = strMsg.indexOf(':');
			if (nPos > 0) {
				lv_nNodeID = (uint8_t)(strMsg.substring(0, nPos).toInt());
			}
		}
		if(nPos > 0)
		{
			// Extract MessageID
			lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1).toInt());
			nPos2 = strMsg.indexOf(':', nPos + 1);
			if (nPos2 > 0) {
				lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1, nPos2).toInt());
				lv_sPayload = strMsg.substring(nPos2 + 1);
			}
		}
	}
	else {
		// Parse serial message
    //LOGD(LOGTAG_MSG, "serial msg: %s", strMsg);
		lv_nMsgID = 0;
		lv_sPayload = strMsg;
	}

	return ProcessSend(lv_nNodeID, lv_nMsgID, lv_sPayload, my_msg, _replyTo, lv_nSubID);
}

bool RF433ServerClass::ProcessSend(String &strMsg, const UC _replyTo, const UC _sensor)
{
	MyMessage tempMsg;
	return ProcessSend(strMsg, tempMsg, _replyTo, _sensor);
}

// ToDo: add message to queue instead of sending out directly
bool RF433ServerClass::ProcessSend(MyMessage *pMsg)
{
	if( !pMsg ) { pMsg = &msg; }

	// Convent message if necessary
	// Add message to sending MQ. Right now tag has no actual purpose (just for debug)
	uint32_t flag = 0;
	flag = ((uint32_t)pMsg->getSensor()<<24) | ((uint32_t)pMsg->getCommand()<<16) | ((uint32_t)pMsg->getType()<<8) | (pMsg->getDestination());
	//LOGD(LOGTAG_MSG, "flag=%d,d=%d,cmd=%d,type=%d,sensor=%d",flag,pMsg->getDestination(),pMsg->getCommand(),pMsg->getType(),pMsg->getSensor());
	if( AddMessage((UC *)&(pMsg->msg), MAX_MESSAGE_LENGTH, GetMQLength(), flag) > 0 ) {
		_times++;
		LOGD(LOGTAG_MSG, "Add sendMQ len:%d", GetMQLength());
		return true;
	}

	LOGW(LOGTAG_MSG, "Failed to add sendMQ");
	return false;
}

// Get messages from RF buffer and store them in MQ
bool RF433ServerClass::PeekMessage()
{
	if( !isValid() ) return false;

	uint8_t from,to = 0;
	uint8_t len;
	MyMessage lv_msg;
	uint8_t *lv_pData = (uint8_t *)&(lv_msg.msg);
	while(available()){
		  len = receive(lv_pData,&from,&to);
			// rough check
			if( len < HEADER_SIZE )
			{
				LOGW(LOGTAG_MSG, "got corrupt dynamic payload!");
				return false;
			} else if( len > MAX_MESSAGE_LENGTH )
			{
				LOGW(LOGTAG_MSG, "message length exceeded: %d", len);
				return false;
			}
			_received++;
			LOGD(LOGTAG_MSG, "Received msg-len=%d, from:%d to:%d sender:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
			len, from, to, lv_msg.getSender(), lv_msg.getDestination(), lv_msg.getCommand(),
			lv_msg.getType(), lv_msg.getSensor(), lv_msg.getLength());
			if( Append(lv_pData, len) <= 0 ) return false;
	}
	return true;
}

// Parse and process message in MQ
bool RF433ServerClass::ProcessReceiveMQ()
{
	bool msgReady;
	UC len, payl_len;
	UC replyTo, _sensor, msgType, transTo;
	bool _bIsAck, _needAck;
	UC *payload;
	UC _bValue;
	US _iValue;
	char strDisplay[SENSORDATA_JSON_SIZE];
	String strTemp;

  while (Length() > 0) {

		msgReady = false;
	  len = Remove(MAX_MESSAGE_LENGTH, msgData);
		payl_len = msg.getLength();
		_sensor = msg.getSensor();
		msgType = msg.getType();
		replyTo = msg.getSender();
		_bIsAck = msg.isAck();
		_needAck = msg.isReqAck();
		payload = (uint8_t *)msg.getCustom();

		/*
	  memset(strDisplay, 0x00, sizeof(strDisplay));
	  msg.getJsonString(strDisplay);
	  SERIAL_LN("  JSON: %s, len: %d", strDisplay, strlen(strDisplay));
	  memset(strDisplay, 0x00, sizeof(strDisplay));
	  SERIAL_LN("  Serial: %s, len: %d", msg.getSerialString(strDisplay), strlen(strDisplay));
		*/
		LOGD(LOGTAG_MSG, "Will process cmd:%d from:%d type:%d sensor:%d",
					msg.getCommand(), replyTo, msgType, _sensor);
  }
  return true;
}

// Scan sendMQ and send messages, repeat if necessary
bool RF433ServerClass::ProcessSendMQ()
{
	MyMessage lv_msg;
	UC *pData = (UC *)&(lv_msg.msg);
	CFastMessageNode *pNode = NULL, *pOld;
	UC pipe, _repeat;
	UC _tag = 0;
	uint32_t _flag = 0;
	bool _remove = false;

	if( GetMQLength() > 0 ) {
		while( pNode = GetMessage(pNode) ) {
			pOld = pNode;
			// Next node
			pNode = pOld->m_pNext;
			// Get message data
			if( pOld->ReadMessage(pData, &_repeat, &_tag, &_flag,15) > 0 )
			{
				// Determine pipe
				if( lv_msg.getCommand() == C_INTERNAL && lv_msg.getType() == I_ID_RESPONSE && lv_msg.isAck() ) {
					//pipe = CURRENT_NODE_PIPE;
				} else if(lv_msg.getType() == I_GET_NONCE_RESPONSE && lv_msg.getDestination() == NODEID_RF_SCANNER)	{
					//pipe = CURRENT_NODE_PIPE;
				} else {
					//pipe = PRIVATE_NET_PIPE;
				}

				// Send message
				_remove = send(lv_msg.getDestination(), lv_msg);
				LOGD(LOGTAG_MSG, "RF-send msg %d-%d tag %d to %d pipe %d tried %d %s", lv_msg.getCommand(), lv_msg.getType(), _tag, lv_msg.getDestination(), pipe, _repeat, _remove ? "OK" : "Failed");

				// Determine whether requires retry
				if( lv_msg.getDestination() == BROADCAST_ADDRESS || IS_GROUP_NODEID(lv_msg.getDestination()) ) {
					if( _remove && _repeat == 1 ) _succ++;
					//_remove = (_repeat > theConfig.GetBcMsgRptTimes());
				} else {
					if( _remove ) _succ++;
					//if( _repeat > theConfig.GetNdMsgRptTimes() ) 	_remove = true;
				}

				// Remove message if succeeded or retried enough times
				if( _remove ) {
					RemoveMessage(pOld);
				}
			}
		}
	}

	return true;
}

bool RF433ServerClass::SendNodeConfig(UC _node, UC _ncf, unsigned int _value)
{
	// Notify Remote Node
	MyMessage lv_msg;
	lv_msg.build(NODEID_GATEWAY, _node, _ncf, C_INTERNAL, I_CONFIG, true);
	lv_msg.set(_value);
	return ProcessSend(&lv_msg);
}

bool RF433ServerClass::SendNodeConfig(UC _node, UC _ncf, UC *_data, const UC _len)
{
	// Notify Remote Node
	MyMessage lv_msg;
	lv_msg.build(NODEID_GATEWAY, _node, _ncf, C_INTERNAL, I_CONFIG, true);
	lv_msg.set((void*)_data, _len);
	return ProcessSend(&lv_msg);
}
