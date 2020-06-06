#pragma once

#include "j2534_v0404.h"
#include <map>
#include "ArduinoComm.h"
#include <vector>
#include <queue>

struct filter {
	unsigned long filterType;
	PASSTHRU_MSG mask = { 0x00 };
	PASSTHRU_MSG pattern = { 0x00 };
	PASSTHRU_MSG control = { 0x00 };
};


class msg_handler
{
public:
	virtual int send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud) = 0;
	int read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
	int add_filter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID);
	int remove_filter(unsigned long filterID);
	virtual void processPayload(DATA_PAYLOAD* p) = 0;
protected:
	static unsigned long free_filter_id;
	char rxBuffer[4096] = { 0x00 };
	char txBuffer[4096] = { 0x00 };
	int rxBufferPos = 0;
	int txBufferPos = 0;
	int protocol_id;
	bool sendResponse = false;
	std::map<unsigned long, filter> filters;

	std::queue<PASSTHRU_MSG> send_queue; // Going to ECU Network
	std::queue<PASSTHRU_MSG> recv_queue; // Going back to application
};


class can_handler : public msg_handler {
public:
	can_handler();
	int send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud);
	void processPayload(DATA_PAYLOAD* p);
};

class iso15765_handler : public msg_handler {
public:
	iso15765_handler();
	int send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud);
	void processPayload(DATA_PAYLOAD* p);
	unsigned long st_min;
	unsigned long bs;
private:
	int internal_send_payload();
	bool inMultiFrame = false;
	int num_bytes = 0;
	int num_bytes_to_send = 0;
	int num_bytes_sent = 0;
	PASSTHRU_MSG temp_ms;
	int frame_num = 0x00;
	bool sending = false;
};

