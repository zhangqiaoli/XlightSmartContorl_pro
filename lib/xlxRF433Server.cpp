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
#include "xlxPublishQueue.h"
#include "xlxAirCondManager.h"

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
	_sentID = 0;
	_ackWaitTick = 0;
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
	EnableRFIRQ();
  return true;
}

bool RF433ServerClass::EnableRFIRQ()
{
	// enable 433 rx/tx interrupt
	attachInterrupt(GDO2, &RF433ServerClass::PeekMessage, this,FALLING);
	return true;
}

bool RF433ServerClass::ChangeNodeID(const uint8_t bNodeID)
{
  //TODO
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
	case 20:
		// Set base config to node
		lv_msg.build(_replyTo, _node, _sensor, C_INTERNAL, I_CONFIG, true);
		payload[0] = 101;  //maindev
		payload[1] = CC1101_433_CHANNEL;
		payload[2] = 101;
		payload[3] = 0;
		nPos = strPayl.indexOf(':');
		Serial.printlnf("msg=%s,pos=%d...",strPayl.c_str(),nPos);
		if (nPos > 0) {
			// Extract brightness, cct or WRGB
			bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
			payload[0] = bytValue;
			payload[2] = bytValue;
			iValue = strPayl.substring(nPos + 1).toInt();
			payload[1] = iValue;
		}
		else
		{
			bytValue = (uint8_t)(strPayl.toInt());
			payload[0] = bytValue;
			payload[2] = bytValue;
		}
		lv_msg.set((void*)payload, 4);
		SERIAL("Now sending set nodeid=%d channel=%d...", payload[0], payload[1]);
		bMsgReady = true;
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
	//LOGD(LOGTAG_MSG, "Will process %s",strMsg);
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
    //
		lv_nMsgID = 0;
		lv_sPayload = strMsg;
	}
	//LOGD(LOGTAG_MSG, "serial msg: %s", strMsg);
  LOGD(LOGTAG_MSG, "Will process nodeid=%d,msgid=%d",lv_nNodeID,lv_nMsgID);
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
	LOGD(LOGTAG_MSG, "flag=%d,d=%d,cmd=%d,type=%d,sensor=%d",flag,pMsg->getDestination(),pMsg->getCommand(),pMsg->getType(),pMsg->getSensor());
	if( AddMessage((UC *)&(pMsg->msg), MAX_MESSAGE_LENGTH, GetMQLength(), flag) > 0 ) {
		_times++;
		LOGD(LOGTAG_MSG, "Add sendMQ len:%d", GetMQLength());
		return true;
	}

	LOGW(LOGTAG_MSG, "Failed to add sendMQ");
	return false;
}

// receive data from RF buffer and store them in MQ
void RF433ServerClass::PeekMessage()
{
	if( !isValid() ) return;
	//detachInterrupt(GDO2);
	uint8_t from,to = 0;
	uint8_t len;
	MyMessage lv_msg;
	uint8_t *lv_pData = (uint8_t *)&(lv_msg.msg);
	if(available()){
		  len = receive(lv_pData,&from,&to);
			if(len > 0)
	    {
				// rough check
				if( len < HEADER_SIZE )
				{
					//LOGW(LOGTAG_MSG, "got corrupt dynamic payload!");
				} else if( len > MAX_MESSAGE_LENGTH )
				{
					//LOGW(LOGTAG_MSG, "message length exceeded: %d", len);
				}
				_received++;
				LOGD(LOGTAG_MSG, "Received isack:%d,msg-len=%d, from:%d to:%d sender:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
				lv_msg.isAck(),len, from, to, lv_msg.getSender(), lv_msg.getDestination(), lv_msg.getCommand(),
				lv_msg.getType(), lv_msg.getSensor(), lv_msg.getLength());
				if(lv_msg.isAck())
				{
					unsigned long ackid = lv_msg.getSender();
					if(_sentID !=0 && ackid == _sentID)
					{
						_sentID = 0;
					}
				}
				Append(lv_pData, len);
			}
	}
	//attachInterrupt(GDO2, &RF433ServerClass::PeekMessage, this, FALLING);
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

		LOGD(LOGTAG_MSG, "Will process cmd:%d from:%d type:%d sensor:%d",
					msg.getCommand(), replyTo, msgType, _sensor);
		switch( msg.getCommand() )
	  {
	    case C_INTERNAL:
	      if( msgType == I_ID_REQUEST ) {
				}
				else if( msgType == I_CONFIG ) {
				   if( _bIsAck ) {
						 if( _sensor == NCF_QUERY ) {
							 theSys.GotNodeConfigAck(replyTo, payload);
						 }
					 }
				 } else if( msgType == I_REBOOT ) {
					 // TODO Reboot node
					 transTo = msg.getDestination();
					 //ListNode<DevStatusRow_t> *DevStatusRowPtr = theSys.SearchDevStatus(transTo);
					 //if( DevStatusRowPtr ) {
						 msg.build(replyTo, transTo, _sensor, C_INTERNAL, I_REBOOT, false);
						 //msg.set((unsigned int)DevStatusRowPtr->data.token);
						 msgReady = true;
					 //}
				 }
				 /*else if( msgType == I_GET_NONCE ) {
				 // RF Scanner Probe
					 if( replyTo == NODEID_RF_SCANNER ) {
						 if( payload[0] == SCANNER_PROBE ) {
							 MsgScanner_ProbeAck();
						 } else if( payload[0] == SCANNER_SETUP_RF ) {
							 transTo = msg.getDestination();
							 if(transTo == NODEID_GATEWAY)
								 Process_SetupRF(payload + 1,payl_len-1);
						 }
						 else if( payload[0] == SCANNER_SETUPDEV_RF ) {
							 uint8_t mac[6] = {0};
							 //WiFi.macAddress(mac);
							 theSys.GetMac(mac);
							 if(isIdentityEqual(payload + 1,mac,sizeof(mac)))
								 Process_SetupRF(payload + 1 + LEN_NODE_IDENTITY, payl_len - 1 - LEN_NODE_IDENTITY);
						 }
					 }
				 }*/
				 break;
			 case C_PRESENTATION:
 				if( _sensor == S_LIGHT || _sensor == S_DIMMER || _sensor == S_ZENSENSOR || _sensor == S_ZENREMOTE ||\
 					  _sensor == S_POWER || _sensor == S_HVAC) {
 					US token;
 					if( _needAck ) {
 						// Presentation message: appear of Smart Lamp
 						// Verify credential, return token if true, and change device status
 						uint64_t nIdentity = msg.getUInt64();
 						token = random(65535);
 						theSys.UpdateNodeList(replyTo,0, msgType, nIdentity,token);
 						if( token ) {
 							// return token
 							// Notes: lampType & S_LIGHT (msgType) are not necessary, use for associated device
 			        msg.build(getAddress(), replyTo, _sensor, C_PRESENTATION, msgType, false, true);
 							msg.set((unsigned int)token);
 							msgReady = true;
 							// ToDo: send status req to this lamp
 						}
 						//theNodeManager.UpdateNode(lv_nNodeID,);
 						LOGN(LOGTAG_MSG, "Node:%d presence!",replyTo);
 					}
 				}
 				else {
 					if( _sensor == S_MOTION || _sensor == S_IR ) {
 						if( msgType == V_STATUS) { // PIR
 							theSys.UpdateMotion(replyTo, _sensor, msg.getByte());
 						}
 					} else if( _sensor == S_LIGHT_LEVEL ) {
 						if( msgType == V_LIGHT_LEVEL) { // ALS
 							theSys.UpdateBrightness(replyTo, msg.getByte());
 						}
 					} else if( _sensor == S_SOUND ) {
 						if( msgType == V_STATUS ) { // MIC
 							theSys.UpdateSound(replyTo, payload[0]);
 						} else if( msgType == V_LEVEL ) {
 							_iValue = payload[1] * 256 + payload[0];
 							theSys.UpdateNoise(replyTo, _iValue);
 						}
 					} else if( _sensor == S_TEMP || _sensor == S_HUM ) {
 						float lv_flt1 = 255, lv_flt2 = 255;
 						if( msgType == V_LEVEL && payl_len >= 4 ) {
 							lv_flt1 = payload[0] + payload[1] / 100.0;
 							lv_flt2 = payload[2] + payload[3] / 100.0;
 						} else if( _sensor == S_TEMP && msgType == V_TEMP ) {
 							lv_flt1 = payload[0] + payload[1] / 100.0;
 						} else if( _sensor == S_HUM && msgType == V_HUM ) {
 							lv_flt2 = payload[0] + payload[1] / 100.0;
 						}
 						theSys.UpdateDHT(replyTo, lv_flt1, lv_flt2);
 					} else if( _sensor == S_DUST || _sensor == S_AIR_QUALITY || _sensor == S_SMOKE ) {
 						if( msgType == V_LEVEL ) { // Dust, Gas or Smoke
 							_iValue = payload[1] * 256 + payload[0];
 							if( _sensor == S_DUST ) {
 								theSys.UpdateDust(replyTo, _iValue);
 							} else if( _sensor == S_AIR_QUALITY ) {
 								if(payl_len >= 10)
 								{
 								  US pm10 = payload[3] * 256 + payload[2];
 									float tvoc = (payload[5] * 256 + payload[4])/10.0;
 									float ch2o = (payload[7] * 256 + payload[6])/10.0;
 									US co2 = payload[9] * 256 + payload[8];
 									theSys.UpdateAirQuality(replyTo, _iValue,pm10,tvoc,ch2o,co2);
 								}
 								else
 								{
 									theSys.UpdateGas(replyTo, _iValue);
 								}
 							} else if( _sensor == S_SMOKE ) {
 								theSys.UpdateSmoke(replyTo, _iValue);
 							}
 						}
 					}
 				}
 				break;
			case C_REQ:
				if( msgType == V_STATUS || msgType == V_PERCENTAGE || msgType == V_LEVEL
					  || msgType == V_RGBW || msgType == V_DISTANCE || msgType == V_VAR1
					  || msgType == V_RELAY_MAP ) {
					transTo = msg.getDestination();
					BOOL bLampDataChanged = false;
					if( _bIsAck ) {
						//SERIAL_LN("REQ ack:%d to: %d 0x%x-0x%x-0x%x-0x%x-0x%x-0x%x-0x%x", msgType, transTo, payload[0],payload[1], payload[2], payload[3], payload[4],payload[5],payload[6]);
						if( msgType == V_STATUS ) {
							if(IS_LAMP_NODEID(replyTo))
							{
								bLampDataChanged |= theSys.ConfirmLampOnOff(replyTo, _sensor,payload[0]);
							}
						}
						if( msgType == V_PERCENTAGE ) {
							bLampDataChanged |= theSys.ConfirmLampBrightness(replyTo, _sensor,payload[0], payload[1]);
						} else if( msgType == V_LEVEL ) {
							bLampDataChanged |= theSys.ConfirmLampCCT(replyTo,_sensor, (US)msg.getUInt());
						} else if( msgType == V_RGBW ) {
							// lamp timing status msg
							if( payload[0] ) {	// Succeed or not
								static bool bFirstRGBW = true;		// Make sure the first message will be sent anyway
								UC _devType = payload[1];	// payload[2] is present status
								UC _ringID = payload[3];
								UC filter = payload[payl_len-1];
								if( IS_SUNNY(_devType) ) {
									// Sunny
									US _CCTValue = payload[7] * 256 + payload[6];
									bLampDataChanged |= theSys.ConfirmLampSunnyStatus(replyTo,_sensor,payload[4], payload[5], _CCTValue, filter,_ringID);
									bLampDataChanged |= bFirstRGBW;
									bFirstRGBW = false;
								} else if( IS_RAINBOW(_devType) || IS_MIRAGE(_devType) ) {
									// Rainbow or Mirage, set RBGW
									bLampDataChanged |= theSys.ConfirmLampHue(replyTo,_sensor,payload[6], payload[7], payload[8], payload[9], _ringID);
									bLampDataChanged |= theSys.ConfirmLampBrightness(replyTo, _sensor, payload[4], payload[5], _ringID);
									bLampDataChanged |= bFirstRGBW;
									bFirstRGBW = false;
								}
							}
						} else if( msgType == V_VAR1 ) { // Change special effect ack
							bLampDataChanged |= theSys.ConfirmLampFilter(replyTo, _sensor, payload[0]);
						} else if( msgType == V_DISTANCE && payload[0] ) {
							UC _devType = payload[1];	// payload[2] is present status
							if( IS_MIRAGE(_devType) ) {
								bLampDataChanged |= theSys.ConfirmLampTop(replyTo, _sensor, payload, payl_len);
							}
						}else if( msgType == V_RELAY_MAP ) {
							// Publish Relay Status
							strTemp = String::format("{'nd':%d,'subid':%d,'km':%d}", replyTo, _sensor,payload[0]);
							theSys.PublishMsg(CLT_ID_DeviceStatus,strTemp.c_str(),strTemp.length(),replyTo,1);
							//LOGD(LOGTAG_MSG, "Recv nd:%d relay msg:%d",replyTo,payload[0]);
							theSys.UpdateNodeList(replyTo,_sensor);

						}

						// If data changed, new status must broadcast to all end points
						if( bLampDataChanged ) {
							transTo = BROADCAST_ADDRESS;
						}
					} /* else { // Request
						// ToDo: verify token
					} */

					// ToDo: if lamp is not present, return error
					if( transTo > 0 ) {
						// Transfer message
						msg.build(replyTo, transTo, _sensor, C_REQ, msgType, _needAck, _bIsAck, true);
						// Keep payload unchanged
						msgReady = true;
					}
				}
				/////////////////// add by zql for airconditioning//////////////////////////////////////////////////////////////
				else if(msgType == V_CURRENT)
				{
					theSys.UpdateNodeList(replyTo,_sensor);
					if(payl_len >= 2)
					{ // electric current change msg
						uint16_t eCurrent = payload[1]<<8 | payload[0];
						UC lv_nNodeID = msg.getSender();
						LOGD(LOGTAG_MSG, "Recv nd:%d current msg:%d",lv_nNodeID,eCurrent);
						theACManager.UpdateACCurrentByNodeid(lv_nNodeID,eCurrent);
					}
				}
				else if(msgType == V_KWH)
				{
					theSys.UpdateNodeList(replyTo,_sensor);
					if(payl_len >= 9)
					{
						uint32_t eQuantity = 0;
						memcpy(&eQuantity,&payload[0],4);
						uint16_t eqindex = payload[5]<<8 | payload[4];
						uint16_t eCurrent = payload[7]<<8 | payload[6];
						uint8_t bReset = payload[8];
						UC lv_nNodeID = msg.getSender();
						LOGD(LOGTAG_MSG, "Recv eq msg,nd:%d,eq:%d,index=%d,current:%d,reset:%d",lv_nNodeID,eQuantity,eqindex,eCurrent,bReset);
						theACManager.UpdateACByNodeid(lv_nNodeID,eCurrent,eQuantity,eqindex,bReset);
						if(_needAck)
						{
							msg.build(transTo, replyTo, _sensor, C_REQ, msgType, 0, 1, true);
							msgReady = true;
						}
					}
					else if(payl_len >= 4)
					{ // electric current change msg
						uint16_t eQuantity = payload[1]<<8 | payload[0];
						uint16_t eqindex = payload[3]<<8 | payload[2];
						uint16_t eCurrent = payload[5]<<8 | payload[4];
						UC lv_nNodeID = msg.getSender();
						LOGD(LOGTAG_MSG, "Recv eq msg,nd:%d,eq:%d,index=%d,current:%d",lv_nNodeID,eQuantity,eqindex,eCurrent);
						theACManager.UpdateACByNodeid(lv_nNodeID,eCurrent,eQuantity,eqindex);
					}
				}
				else if(msgType == V_HVAC_FLOW_STATE)
				{
					theSys.UpdateNodeList(replyTo,_sensor);
					if(payl_len >= 4)
					{ // electric current change msg
						uint8_t onoff = payload[0];
						uint8_t mode = payload[1];
						uint8_t temp = payload[2];
						uint8_t fanlevel = payload[3];
						UC lv_nNodeID = msg.getSender();
						LOGD(LOGTAG_MSG, "Recv acstatus msg,nd:%d,onoff:%d,mode=%d,temp:%d,fanlevel:%d",onoff,mode,temp,fanlevel);
						theACManager.UpdateACStatusByNodeid(lv_nNodeID,onoff,mode,temp,fanlevel);
					}
				}
				/////////////////// add by zql for airconditioning end//////////////////////////////////////////////////////////
				break;

			case C_SET:
				break;

	    default:
	      break;
	  }
  }
  return true;
}
// TODO
// Scan sendMQ and send messages, repeat if necessary
bool RF433ServerClass::ProcessSendMQ()
{
	MyMessage lv_msg;
	MyMessage resmsg;
	uint8_t reslen = 0;
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
				// Send message
				detachInterrupt(GDO2);
				_remove = send(lv_msg.getDestination(), lv_msg);
				attachInterrupt(GDO2, &RF433ServerClass::PeekMessage, this, FALLING);
	      _sentID = lv_msg.getDestination();
				LOGD(LOGTAG_MSG, "RF-send msg %d-%d tag %d to %d tried %d id=%d", lv_msg.getCommand(), lv_msg.getType(), _tag, lv_msg.getDestination(), _repeat,_sentID);
				if( lv_msg.getDestination() == BROADCAST_ADDRESS || lv_msg.getDestination() == BROADCAST_ADDRESS1)
				{
          _remove = (_repeat > theConfig.GetBcMsgRptTimes());
				}
				else
				{
					if(lv_msg.getCommand() == C_INTERNAL && lv_msg.getType() == I_CONFIG)
					{
						_remove = (_repeat > theConfig.GetNdMsgRptTimes());
					}
					else
					{
						_remove = false;
						// wait for ack
						while(_ackWaitTick < ACK_TIMEOUT)
						{
							if(_sentID != 0)
							{// wait for ack
								_ackWaitTick++;                               //increment ACK wait counter
								delay(1);
							}
							else
							{
								_remove = true;
								break;
							}
						}
						_ackWaitTick = 0;
						if( !_remove && _repeat > theConfig.GetNdMsgRptTimes() ) 	_remove = true;
					}
					/*if(reslen > 0)
					{
						LOGD(LOGTAG_MSG, "Receive real-time msg len=%d sender:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
							 reslen,resmsg.getSender(), resmsg.getDestination(), resmsg.getCommand(),
							 resmsg.getType(), resmsg.getSensor(), resmsg.getLength());
						uint8_t *lv_pData = (uint8_t *)&(resmsg.msg);
						Append(lv_pData, reslen);
					}
					if( _repeat > theConfig.GetNdMsgRptTimes() ) 	_remove = true;*/
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
