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
// TODO
bool RF433ServerClass::ProcessSend(const UC _node, const UC _msgID, String &strPayl, MyMessage &my_msg, const UC _replyTo, const UC _sensor)
{
	bool sentOK = false;
	bool bMsgReady = false;
	uint8_t bytValue;
	int iValue;
	float fValue;
	char strBuffer[64];
	uint8_t payload[MAX_PAYLOAD];
	MyMessage lv_msg;
	int nPos;

	switch (_msgID)
	{
	case 0: // Free style
		iValue = min(strPayl.length(), 63);
		strncpy(strBuffer, strPayl.c_str(), iValue);
		strBuffer[iValue] = 0;
		// Serail format to MySensors message structure
		bMsgReady = serialMsgParser.parse(lv_msg, strBuffer);
		if (bMsgReady) {
			if( _sensor > 0 ) lv_msg.setSensor(_sensor);
			SERIAL("Now sending message...");
		}
		break;

	case 1:   // Request new node ID
		if( _node == GATEWAY_ADDRESS ) {
			SERIAL_LN("Controller can not request node ID\n\r");
		} else {
			// Set specific NodeID to node
			UC newID = 0;
			if( strPayl.length() > 0 ) {
				newID = (UC)strPayl.toInt();
			}
			if( newID > 0 ) {
				lv_msg.build(_replyTo, _node, newID, C_INTERNAL, I_ID_RESPONSE, false, false);
				//lv_msg.set(getMyNetworkID());
				//theConfig.lstNodes.clearNodeId(_node);
				SERIAL("Now sending new id:%d to node:%d...", newID, _node);
				bMsgReady = true;
			} else {
				// Reboot node
				ListNode<DevStatusRow_t> *DevStatusRowPtr = theSys.SearchDevStatus(_node);
				if( DevStatusRowPtr ) {
					lv_msg.build(_replyTo, _node, _sensor, C_INTERNAL, I_REBOOT, false);
					lv_msg.set((unsigned int)DevStatusRowPtr->data.token);
					bMsgReady = true;
				}
			}
		}
		break;

	case 2:   // Node Config
		{
			nPos = strPayl.indexOf(':');
			if (nPos > 0) {
				bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
				iValue = strPayl.substring(nPos + 1).toInt();
				lv_msg.build(_replyTo, _node, bytValue, C_INTERNAL, I_CONFIG, true);
				lv_msg.set((unsigned int)iValue);
				bMsgReady = true;
				SERIAL("Now sending node:%d config:%d value:%d...", _node, bytValue, iValue);
			}
		}
		break;

	case 3:   // Temperature sensor present with sensor id 1, req no ack
		lv_msg.build(_replyTo, _node, _sensor, C_PRESENTATION, S_TEMP, false);
		lv_msg.set("");
		bMsgReady = true;
		SERIAL("Now sending DHT11 present message...");
		break;

	case 6:   // Get main lamp(ID:1) power(V_STATUS:2) on/off, ack
		lv_msg.build(_replyTo, _node, _sensor, C_REQ, V_STATUS, true);
		bMsgReady = true;
		SERIAL("Now sending get V_STATUS message...");
		break;

	case 7:   // Set main lamp(ID:1) power(V_STATUS:2) on/off, ack
		lv_msg.build(_replyTo, _node, _sensor, C_SET, V_STATUS, true);
		bytValue = constrain(strPayl.toInt(), DEVICE_SW_OFF, DEVICE_SW_TOGGLE);
		lv_msg.set(bytValue);
		bMsgReady = true;
		SERIAL("Now sending set V_STATUS %s message...", (bytValue ? "on" : "off"));
		break;

	case 8:   // Get main lamp(ID:1) dimmer (V_PERCENTAGE:3), ack
		lv_msg.build(_replyTo, _node, _sensor, C_REQ, V_PERCENTAGE, true);
		bMsgReady = true;
		SERIAL("Now sending get V_PERCENTAGE message...");
		break;

	case 9:   // Set main lamp(ID:1) dimmer (V_PERCENTAGE:3), ack
		lv_msg.build(_replyTo, _node, _sensor, C_SET, V_PERCENTAGE, true);
		bytValue = constrain(strPayl.toInt(), 0, 100);
		lv_msg.set((uint8_t)OPERATOR_SET, bytValue);
		bMsgReady = true;
		SERIAL("Now sending set V_PERCENTAGE:%d message...", bytValue);
		break;

	case 10:  // Get main lamp(ID:1) color temperature (V_LEVEL), ack
		lv_msg.build(_replyTo, _node, _sensor, C_REQ, V_LEVEL, true);
		bMsgReady = true;
		SERIAL("Now sending get CCT V_LEVEL message...");
		break;

	case 11:  // Set main lamp(ID:1) color temperature (V_LEVEL), ack
		lv_msg.build(_replyTo, _node, _sensor, C_SET, V_LEVEL, true);
		iValue = constrain(strPayl.toInt(), CT_MIN_VALUE, CT_MAX_VALUE);
		lv_msg.set((uint8_t)OPERATOR_SET, (unsigned int)iValue);
		bMsgReady = true;
		SERIAL("Now sending set CCT V_LEVEL %d message...", iValue);
		break;

	case 12:  // Request lamp status in one
		lv_msg.build(_replyTo, _node, _sensor, C_REQ, V_RGBW, true);
		lv_msg.set((uint8_t)RING_ID_ALL);		// RING_ID_1 is also workable currently
		bMsgReady = true;
		SERIAL("Now sending get dev-status (V_RGBW) message...");
		break;

	case 13:  // Set main lamp(ID:1) status in one, ack
		lv_msg.build(_replyTo, _node, _sensor, C_SET, V_RGBW, true);
		payload[0] = RING_ID_ALL;
		payload[1] = 1;
		payload[2] = 65;
		nPos = strPayl.indexOf(':');
		if (nPos > 0) {
			// Extract brightness, cct or WRGB
			bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
			payload[2] = bytValue;
			iValue = strPayl.substring(nPos + 1).toInt();
			if( iValue < 256 ) {
				// WRGB
				payload[3] = iValue;	// W
				payload[4] = 0;	// R
				payload[5] = 0;	// G
				payload[6] = 0;	// B
				for( int cindex = 3; cindex < 7; cindex++ ) {
					strPayl = strPayl.substring(nPos + 1);
					nPos = strPayl.indexOf(':');
					if (nPos <= 0) {
						bytValue = (uint8_t)(strPayl.toInt());
						payload[cindex] = bytValue;
						break;
					}
					bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
					payload[cindex] = bytValue;
				}
				lv_msg.set((void*)payload, 7);
				SERIAL("Now sending set BR=%d WRGB=(%d,%d,%d,%d)...",
						payload[2], payload[3], payload[4], payload[5], payload[6]);
			} else {
				// CCT
				iValue = constrain(iValue, CT_MIN_VALUE, CT_MAX_VALUE);
				payload[3] = iValue % 256;
			  payload[4] = iValue / 256;
				lv_msg.set((void*)payload, 5);
				SERIAL("Now sending set BR=%d CCT=%d...", bytValue, iValue);
			}
		} else {
			iValue = 3000;
			payload[3] = iValue % 256;
		  payload[4] = iValue / 256;
			lv_msg.set((void*)payload, 5);
			SERIAL("Now sending set BR=%d CCT=%d...", bytValue, iValue);
		}
		bMsgReady = true;
		break;

	case 14:	// Reserved for query command
	case 16:	// Reserved for query command
		break;

	case 15:	// Set Device Scenerio
		bytValue = (UC)(strPayl.toInt());
		theSys.ChangeLampScenario(_node, bytValue, _replyTo, _sensor);
		break;

	case 17:	// Set special effect
		lv_msg.build(_replyTo, _node, _sensor, C_SET, V_VAR1, true);
		bytValue = (UC)(strPayl.toInt());
		lv_msg.set(bytValue);
		bMsgReady = true;
		SERIAL("Now setting special effect %d...", bytValue);
		break;
	}

	if (bMsgReady) {
		SERIAL("to %d...", lv_msg.getDestination());
		sentOK = ProcessSend(&lv_msg);
		my_msg = lv_msg;
		SERIAL_LN(sentOK ? "OK" : "failed");
	}

	return sentOK;
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
// TODO
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
// TODO
// Scan sendMQ and send messages, repeat if necessary
bool RF433ServerClass::ProcessSendMQ()
{
	MyMessage lv_msg;
	UC *pData = (UC *)&(lv_msg.msg);
	CFastMessageNode *pNode = NULL, *pOld;
	UC _repeat;
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
				LOGD(LOGTAG_MSG, "RF-send msg %d-%d tag %d to %d tried %d %s", lv_msg.getCommand(), lv_msg.getType(), _tag, lv_msg.getDestination(), _repeat, _remove ? "OK" : "Failed");

				// Determine whether requires retry
				if( lv_msg.getDestination() == BROADCAST_ADDRESS || IS_GROUP_NODEID(lv_msg.getDestination()) ) {
					if( _remove && _repeat == 1 ) _succ++;
					_remove = (_repeat > theConfig.GetBcMsgRptTimes());
				} else {
					if( _remove ) _succ++;
					if( _repeat > theConfig.GetNdMsgRptTimes() ) 	_remove = true;
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
