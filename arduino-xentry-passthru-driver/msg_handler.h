#pragma once

#include "j2534_v0404.h"
#include <map>

class filter {
public:
	unsigned long filterType;
	PASSTHRU_MSG* mask;
	PASSTHRU_MSG* pattern;
	PASSTHRU_MSG* control;
};


class msg_handler
{
public:
	virtual int send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud) = 0;
	virtual int read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout) = 0;
	int add_filter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID);
	int remove_filter(unsigned long filterID);
protected:
	static unsigned long free_filter_id;
	char rxBuffer[4096] = { 0x00 };
	char txBuffer[4096] = { 0x00 };
	int rxBufferPos = 0;
	int txBufferPos = 0;
	int protocol_id;
	std::map<unsigned long, filter> filters;
};


class can_handler : public msg_handler {
public:
	can_handler();
	int send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud);
	int read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
};

class iso15765_handler : public msg_handler {
public:
	iso15765_handler();
	int send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud);
	int read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
	unsigned long st_min;
	unsigned long bs;
private:
	int internal_send_payload(PASSTHRU_MSG* m);
};

