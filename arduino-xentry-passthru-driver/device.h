#pragma once

#include <map>
#include "msg_handler.h"
#include "ArduinoComm.h"

class device
{
public:
	device(unsigned long id);
	unsigned long id;
private:
};

class device_channel {
public:
	device_channel(unsigned long devID);
	void setFlags(unsigned long flags);
	void setProtocol(unsigned long protocol);
	void setBaudRate(unsigned long baud);
	void closeChannel();
	int send_message(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
	int handleIOCTL(unsigned long IoctlID, void* pInput, void* pOutput);
	int add_filter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID);
	int rem_filter(unsigned long filterID);
	void recvPayload(DATA_PAYLOAD *p);
	int read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
	void update_channel();
private:
	msg_handler *handler;
	unsigned long deviceID;
	unsigned long baud;
	unsigned long flags;
	unsigned long protocol;
	void getConfig(SCONFIG* c);
	void setConfig(SCONFIG* c);
};

class device_table {
public:
	device_table();
	void update_channels();
	static unsigned long free_id;
	static unsigned long free_chan_id;
	int remove_device(unsigned long id);
	int add_device(unsigned long id);
	int add_channel(unsigned long deviceID, unsigned long protocolID, unsigned long Flags, unsigned long baudRate, unsigned long* pChannelID);
	int remove_channel(unsigned long chanID);
	device* getDev(unsigned long id);
	device_channel* getChannel(unsigned long id);
	void processPayload(DATA_PAYLOAD* msg);
private:
	std::map<unsigned long, device> device_list;
	std::map<unsigned long, device_channel> channels;
};

extern device_table dev_map;