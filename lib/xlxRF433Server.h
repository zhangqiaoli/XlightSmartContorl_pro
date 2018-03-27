//  xlxRF24Server.h - Xlight RF2.4 server library

#ifndef xlxRF433Server_h
#define xlxRF433Server_h

#include "DataQueue.h"
#include "MessageQ.h"
#include "MyTransport433.h"

// RF433 Server class
class RF433ServerClass : public MyTransport433, public CDataQueue, public CFastMessageQ
{
public:
  RF433ServerClass();

  bool ServerBegin(uint8_t channel,uint8_t address);

  bool ProcessSend(const UC _node, const UC _msgID, String &strPayl, MyMessage &my_msg, const UC _replyTo, const UC _sensor = 0);
  bool ProcessSend(String &strMsg, MyMessage &my_msg, const UC _replyTo = 0, const UC _sensor = 0);
  bool ProcessSend(String &strMsg, const UC _replyTo = 0, const UC _sensor = 0); //overloaded
  bool ProcessSend(MyMessage *pMsg = NULL);

  bool ProcessMQ();
  bool ProcessSendMQ();
  bool ProcessReceiveMQ();

  bool PeekMessage();

  unsigned long _times;
  unsigned long _succ;
  unsigned long _received;

};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern RF433ServerClass theRadio;

#endif /* xlxRF433Server_h */